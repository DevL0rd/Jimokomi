local Class = include("src/classes/Class.lua")
local Screen = include("src/classes/Screen.lua")

local function countLines(text)
	local count = 1
	for i = 1, #text do
		if sub(text, i, i) == "\n" then
			count = count + 1
		end
	end
	return count
end

local function tableToString(tbl, indent, visited)
	indent = indent or 0
	visited = visited or {}

	if visited[tbl] then
		return "[circular reference]"
	end
	visited[tbl] = true

	local result = "{\n"
	local spacing = ""

	for i = 1, indent + 1 do
		spacing = spacing .. "  "
	end

	local has_items = false

	for k, v in pairs(tbl) do
		has_items = true
		local key_str = type(k) == "string" and k or "[" .. tostr(k) .. "]"

		if type(v) == "table" then
			result = result .. spacing .. key_str .. " = " .. tableToString(v, indent + 1, visited) .. ",\n"
		elseif type(v) == "string" then
			result = result .. spacing .. key_str .. " = \"" .. v .. "\",\n"
		elseif type(v) == "function" then
			result = result .. spacing .. key_str .. " = [function],\n"
		else
			result = result .. spacing .. key_str .. " = " .. tostr(v) .. ",\n"
		end
	end

	if not has_items then
		visited[tbl] = nil
		return "{}"
	end

	local closing_spacing = ""
	for i = 1, indent do
		closing_spacing = closing_spacing .. "  "
	end

	result = result .. closing_spacing .. "}"
	visited[tbl] = nil
	return result
end

local DebugBuffer = Class:new({
	_type = "DebugBuffer",
	text = "",
	line_count = 0,
	max_lines = nil,

	init = function(self)
		self.text = ""
		self.line_count = 0
		if self.max_lines == nil then
			self.max_lines = flr(Screen.h / 9)
		end
	end,

	clear = function(self)
		self.text = ""
		self.line_count = 0
	end,

	setMaxLines = function(self, max_lines)
		self.max_lines = max_lines
	end,

	append = function(self, value)
		if self.line_count >= self.max_lines then
			if self.line_count == self.max_lines then
				self.text = self.text .. "... (debug output truncated)\n"
				self.line_count = self.line_count + 1
			end
			return
		end

		local text_to_add = type(value) == "table" and (tableToString(value) .. "\n") or (tostr(value) .. "\n")
		local new_lines = countLines(text_to_add)

		if self.line_count + new_lines > self.max_lines then
			local remaining_lines = self.max_lines - self.line_count

			if remaining_lines > 0 then
				local current_line = ""
				local lines = {}

				for i = 1, #text_to_add do
					local char = sub(text_to_add, i, i)
					if char == "\n" then
						add(lines, current_line)
						current_line = ""
					else
						current_line = current_line .. char
					end
				end

				if current_line ~= "" then
					add(lines, current_line)
				end

				for i = 1, min(remaining_lines, #lines) do
					self.text = self.text .. lines[i] .. "\n"
					self.line_count = self.line_count + 1
				end
			end

			if self.line_count < self.max_lines then
				self.text = self.text .. "... (debug output truncated)\n"
				self.line_count = self.max_lines + 1
			end

			return
		end

		self.text = self.text .. text_to_add
		self.line_count = self.line_count + new_lines
	end,
})

return DebugBuffer
