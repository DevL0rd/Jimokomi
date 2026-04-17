local raw_include = include
local include_cache = {}
local include_in_progress = {}

local function normalize_include_path(path)
	return path
end

function include(path)
	local normalized_path = normalize_include_path(path)

	if include_cache[normalized_path] ~= nil then
		return include_cache[normalized_path]
	end

	if include_in_progress[normalized_path] then
		error("circular include: " .. normalized_path)
	end

	include_in_progress[normalized_path] = true
	local result = raw_include(normalized_path)
	include_in_progress[normalized_path] = nil
	include_cache[normalized_path] = result
	return result
end

function random_float(min_val, max_val)
	return min_val + rnd(max_val - min_val)
end

function random_int(min_val, max_val)
	return flr(min_val + rnd(max_val - min_val + 1))
end

function round(num, decimal_places)
	if decimal_places == nil then
		return flr(num + 0.5)
	else
		local factor = 10 ^ decimal_places
		return flr(num * factor + 0.5) / factor
	end
end
