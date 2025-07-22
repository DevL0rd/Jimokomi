local Layer = include("src/classes/Layer.lua")
local Vector = include("src/classes/Vector.lua")
local Screen = include("src/classes/Screen.lua")

local last_time = 0
_dt = 0
local debug_text = ""
local debug_line_count = 0
local max_debug_lines = flr(Screen.h / 9) -- Auto-calculate based on screen height (9px per line)

-- Helper function to count lines in a string
local function count_lines(text)
	local count = 1
	for i = 1, #text do
		if sub(text, i, i) == "\n" then
			count = count + 1
		end
	end
	return count
end

-- Helper function to serialize tables for pretty printing
local function table_to_string(tbl, indent, visited)
	indent = indent or 0
	visited = visited or {}

	-- Prevent infinite recursion with circular references
	if visited[tbl] then
		return "[circular reference]"
	end
	visited[tbl] = true

	local result = "{\n"

	-- Create spacing string manually
	local spacing = ""
	for i = 1, indent + 1 do
		spacing = spacing .. "  "
	end

	local has_items = false

	for k, v in pairs(tbl) do
		has_items = true
		local key_str = type(k) == "string" and k or "[" .. tostr(k) .. "]"

		if type(v) == "table" then
			result = result .. spacing .. key_str .. " = " .. table_to_string(v, indent + 1, visited) .. ",\n"
		elseif type(v) == "string" then
			result = result .. spacing .. key_str .. " = \"" .. v .. "\",\n"
		elseif type(v) == "function" then
			result = result .. spacing .. key_str .. " = [function],\n"
		else
			result = result .. spacing .. key_str .. " = " .. tostr(v) .. ",\n"
		end
	end

	if not has_items then
		return "{}"
	end

	-- Create closing spacing
	local closing_spacing = ""
	for i = 1, indent do
		closing_spacing = closing_spacing .. "  "
	end

	result = result .. closing_spacing .. "}"
	visited[tbl] = nil -- Clean up for potential reuse
	return result
end

function print_debug(text)
	-- Check if we've reached the line limit
	if debug_line_count >= max_debug_lines then
		-- If this is the first time we hit the limit, add a truncation message
		if debug_line_count == max_debug_lines then
			debug_text = debug_text .. "... (debug output truncated)\n"
			debug_line_count = debug_line_count + 1
		end
		return -- Don't add any more debug text
	end

	local text_to_add
	if type(text) == "table" then
		text_to_add = table_to_string(text) .. "\n"
	else
		text_to_add = tostr(text) .. "\n"
	end

	-- Count how many lines this would add
	local new_lines = count_lines(text_to_add)

	-- If adding this would exceed our limit, try to fit what we can
	if debug_line_count + new_lines > max_debug_lines then
		local remaining_lines = max_debug_lines - debug_line_count

		if remaining_lines > 0 then
			-- Split the text into lines
			local lines = {}
			local current_line = ""
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

			-- Add as many lines as we can fit
			for i = 1, min(remaining_lines, #lines) do
				debug_text = debug_text .. lines[i] .. "\n"
				debug_line_count = debug_line_count + 1
			end
		end

		-- Add truncation message if we haven't already and have space
		if debug_line_count < max_debug_lines then
			debug_text = debug_text .. "... (debug output truncated)\n"
			debug_line_count = max_debug_lines + 1
		end
	else
		-- We can fit this text, add it normally
		debug_text = debug_text .. text_to_add
		debug_line_count = debug_line_count + new_lines
	end
end

-- Function to clear debug text and reset line count
function clear_debug()
	debug_text = ""
	debug_line_count = 0
end

-- Function to set maximum debug lines (useful for different screen sizes)
function set_max_debug_lines(max_lines)
	max_debug_lines = max_lines
end

local Engine = {
	layers = {},
	master_camera = nil,
	debug = DEBUG or false,
	w = 16 * 64,
	h = 16 * 32,
	gravity = Vector:new({
		y = 200
	}),
	fill_color = 1,
	stroke_color = 5,
	createLayer = function(self, layer_id, physics_enabled, map_id)
		physics_enabled = physics_enabled or true
		local layer = Layer:new()
		layer.debug = self.debug
		layer.layer_id = layer_id
		layer.physics_enabled = physics_enabled
		layer.gravity = self.gravity
		layer.map_id = map_id

		layer.w = self.w
		layer.h = self.h

		layer.camera:setBounds(0, self.w - Screen.w, 0, self.h - Screen.h)

		self.layers[layer_id] = layer

		if not self.master_camera then
			self.master_camera = layer.camera
		end

		return layer
	end,

	getLayer = function(self, layer_id)
		return self.layers[layer_id]
	end,

	linkCameras = function(self, layer_id, parallax_x, parallax_y)
		local layer = self.layers[layer_id]
		if layer and self.master_camera then
			layer.camera:linkToCamera(self.master_camera, parallax_x, parallax_y)
		end
	end,

	setMasterCamera = function(self, layer_id)
		local layer = self.layers[layer_id]
		if layer then
			self.master_camera = layer.camera
		end
	end,

	update = function(self)
		_dt = time() - last_time
		last_time = time()
		debug_text = ""
		debug_line_count = 0 -- Reset debug line count each frame

		if self.master_camera then
			self.master_camera:update()
		end

		for layer_id, world in pairs(self.layers) do
			if world.camera ~= self.master_camera and world.camera.parallax_factor then
				world.camera:linkToCamera(self.master_camera,
					world.camera.parallax_factor.x,
					world.camera.parallax_factor.y)
			end

			world:update()
		end
	end,

	draw = function(self)
		cls()

		if self.master_camera then
			local cam_screen = self.master_camera:worldToScreen({ x = 0, y = 0 })

			if self.fill_color > -1 then
				rectfill(cam_screen.x, cam_screen.y,
					cam_screen.x + self.w - 1,
					cam_screen.y + self.h - 1,
					self.fill_color)
			end

			if self.stroke_color > -1 then
				rect(cam_screen.x, cam_screen.y,
					cam_screen.x + self.w - 1,
					cam_screen.y + self.h - 1,
					self.stroke_color)
			end
		end

		local layer_keys = {}
		for k, v in pairs(self.layers) do
			add(layer_keys, k)
		end

		for i = 1, #layer_keys - 1 do
			for j = 1, #layer_keys - i do
				if layer_keys[j] > layer_keys[j + 1] then
					local temp = layer_keys[j]
					layer_keys[j] = layer_keys[j + 1]
					layer_keys[j + 1] = temp
				end
			end
		end

		for _, layer_id in pairs(layer_keys) do
			self.layers[layer_id]:draw()
		end
		if self.debug then
			self:debug_state()
			print(debug_text, 1, 1) -- Print debug text at the top-left corner
		end
		-- Reset camera after drawing all layers
		camera()
	end,

	start = function(self)
		last_time = time()
		for _, world in pairs(self.layers) do
			world:start()
		end
	end,

	stop = function(self)
		for _, world in pairs(self.layers) do
			world:stop()
		end
	end,

	-- Debug helper functions
	debug_state = function(self)
		if not self.debug then return end

		local state = {
			layers = {},
			master_camera = nil,
			world_size = { w = self.w, h = self.h }
		}

		-- Get layer info
		for layer_id, layer in pairs(self.layers) do
			state.layers[layer_id] = {
				entity_count = #layer.entities,
				running = layer.running,
				physics_enabled = layer.physics_enabled,
				map_id = layer.map_id
			}
		end

		-- Get master camera info
		if self.master_camera then
			state.master_camera = {
				pos = { x = self.master_camera.pos.x, y = self.master_camera.pos.y },
				has_target = self.master_camera.target ~= nil,
				shake_intensity = self.master_camera.shake.intensity
			}
		end

		print_debug("Engine State:")
		print_debug(state)
	end,

	debug_layer = function(self, layer_id)
		if not self.debug then return end

		local layer = self.layers[layer_id]
		if not layer then
			print_debug("Layer " .. layer_id .. " not found")
			return
		end

		local layer_info = {
			layer_id = layer_id,
			entity_count = #layer.entities,
			running = layer.running,
			physics_enabled = layer.physics_enabled,
			map_id = layer.map_id,
			camera_pos = { x = layer.camera.pos.x, y = layer.camera.pos.y },
			entities = {}
		}

		-- Get entity info (limited to prevent spam)
		for i = 1, min(5, #layer.entities) do
			local entity = layer.entities[i]
			layer_info.entities[i] = {
				type = entity._type or "unknown",
				pos = entity.pos and { x = entity.pos.x, y = entity.pos.y } or nil
			}
		end

		if #layer.entities > 5 then
			layer_info.entities["..."] = "(" .. (#layer.entities - 5) .. " more entities)"
		end

		print_debug("Layer " .. layer_id .. ":")
		print_debug(layer_info)
	end
}

return Engine
