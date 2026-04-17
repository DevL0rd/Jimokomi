local Vector = include("src/Engine/Math/Vector.lua")

local WorldSampling = {}

local function build_bounds_key(bounds)
	if not bounds then
		return "nil"
	end
	return table.concat({
		tostr(bounds.left or 0),
		tostr(bounds.top or 0),
		tostr(bounds.right or 0),
		tostr(bounds.bottom or 0),
	}, ":")
end

local function build_rule_key(self, prefix, rule, radius)
	rule = rule or {}
	local bounds = self:getQueryBounds(rule)
	local tile_ids = {}
	for i = 1, #(rule.tile_ids or {}) do
		tile_ids[i] = tostr(rule.tile_ids[i])
	end
	return table.concat({
		prefix or "rule",
		tostr(self.layer and self.layer.map_id or 0),
		build_bounds_key(bounds),
		tostr(rule.kind or "map_random"),
		tostr(rule.scope or "map"),
		tostr(rule.margin or 0),
		tostr(rule.padding or 0),
		tostr(rule.attempts or 0),
		tostr(radius or 0),
		table.concat(tile_ids, ","),
	}, "|")
end

local function copy_point(pos)
	if not pos then
		return nil
	end
	return Vector:new({ x = pos.x, y = pos.y })
end

local function get_cached_list(self, key, builder)
	self.sampling_cache = self.sampling_cache or {}
	local cached = self.sampling_cache[key]
	if cached then
		return cached
	end
	cached = builder() or {}
	self.sampling_cache[key] = cached
	return cached
end

WorldSampling.getRandomPointInBounds = function(self, bounds)
	if not bounds or bounds.left > bounds.right or bounds.top > bounds.bottom then
		return nil
	end
	return Vector:new({
		x = random_int(bounds.left, bounds.right),
		y = random_int(bounds.top, bounds.bottom)
	})
end

WorldSampling.getRandomMapPosition = function(self, rule, require_offscreen, radius)
	rule = rule or {}
	radius = radius or 0
	local attempts = rule.attempts or 24
	local padding = rule.padding or 0
	local bounds = self:getQueryBounds(rule)

	for _ = 1, attempts do
		local candidate = self:getRandomPointInBounds(bounds)
		if candidate and (not require_offscreen or not self:isOnScreen(candidate, radius, padding)) then
			return candidate
		end
	end

	if require_offscreen then
		return nil
	end

	return self:getRandomPointInBounds(bounds)
end

WorldSampling.getRandomSurfacePosition = function(self, rule, require_offscreen, radius)
	rule = rule or {}
	radius = radius or 0
	local bounds = self:getQueryBounds(rule)
	local attempts = rule.attempts or 48
	local padding = rule.padding or 0
	local tile_size = self.layer.tile_size or 16
	local cache_key = build_rule_key(self, "surface", rule, radius)
	local candidates = get_cached_list(self, cache_key, function()
		local values = {}
		for x = bounds.left, bounds.right, tile_size do
			local candidate = self:findSurfacePositionAtX(x, radius, rule)
			if candidate then
				add(values, candidate)
			end
		end
		return values
	end)
	if #candidates == 0 then
		return nil
	end

	for _ = 1, attempts do
		local candidate = candidates[random_int(1, #candidates)]
		if candidate and (not require_offscreen or not self:isOnScreen(candidate, radius, padding)) then
			return copy_point(candidate)
		end
	end

	if require_offscreen then
		for i = 1, #candidates do
			local candidate = candidates[i]
			if not self:isOnScreen(candidate, radius, padding) then
				return copy_point(candidate)
			end
		end
		return nil
	end

	return copy_point(candidates[random_int(1, #candidates)])
end

WorldSampling.getRandomTilePosition = function(self, rule, require_offscreen, radius)
	rule = rule or {}
	radius = radius or 0
	local positions = get_cached_list(self, build_rule_key(self, "tile", rule, radius), function()
		return self:collectTilePositions(rule.tile_ids or {}, rule)
	end)
	if #positions == 0 then
		return nil
	end

	local attempts = rule.attempts or 24
	local padding = rule.padding or 0

	for _ = 1, attempts do
		local candidate = positions[random_int(1, #positions)]
		if not require_offscreen or not self:isOnScreen(candidate, radius, padding) then
			return copy_point(candidate)
		end
	end

	if require_offscreen then
		for _, candidate in pairs(positions) do
			if not self:isOnScreen(candidate, radius, padding) then
				return copy_point(candidate)
			end
		end
		return nil
	end

	local fallback = positions[random_int(1, #positions)]
	return copy_point(fallback)
end

WorldSampling.resolveRule = function(self, rule, require_offscreen, radius)
	rule = rule or {}
	local kind = rule.kind or "map_random"

	if kind == "surface_random" then
		return self:getRandomSurfacePosition(rule, require_offscreen, radius)
	end
	if kind == "tile_random" then
		return self:getRandomTilePosition(rule, require_offscreen, radius)
	end
	return self:getRandomMapPosition(rule, require_offscreen, radius)
end

return WorldSampling
