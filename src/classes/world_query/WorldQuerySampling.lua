local Vector = include("src/classes/Vector.lua")

local WorldQuerySampling = {}

WorldQuerySampling.getRandomPointInBounds = function(self, bounds)
	if not bounds or bounds.left > bounds.right or bounds.top > bounds.bottom then
		return nil
	end
	return Vector:new({
		x = random_int(bounds.left, bounds.right),
		y = random_int(bounds.top, bounds.bottom)
	})
end

WorldQuerySampling.getRandomMapPosition = function(self, rule, require_offscreen, radius)
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

WorldQuerySampling.getRandomSurfacePosition = function(self, rule, require_offscreen, radius)
	rule = rule or {}
	radius = radius or 0
	local bounds = self:getQueryBounds(rule)
	local attempts = rule.attempts or 48
	local padding = rule.padding or 0
	local tile_size = self.layer.tile_size or 16

	for _ = 1, attempts do
		local x = random_int(bounds.left, bounds.right)
		local candidate = self:findSurfacePositionAtX(x, radius, rule)
		if candidate and (not require_offscreen or not self:isOnScreen(candidate, radius, padding)) then
			return candidate
		end
	end

	for x = bounds.left, bounds.right, tile_size do
		local candidate = self:findSurfacePositionAtX(x, radius, rule)
		if candidate and (not require_offscreen or not self:isOnScreen(candidate, radius, padding)) then
			return candidate
		end
	end

	return nil
end

WorldQuerySampling.getRandomTilePosition = function(self, rule, require_offscreen, radius)
	rule = rule or {}
	radius = radius or 0
	local positions = self:collectTilePositions(rule.tile_ids or {}, rule)
	if #positions == 0 then
		return nil
	end

	local attempts = rule.attempts or 24
	local padding = rule.padding or 0

	for _ = 1, attempts do
		local candidate = positions[random_int(1, #positions)]
		if not require_offscreen or not self:isOnScreen(candidate, radius, padding) then
			return Vector:new({ x = candidate.x, y = candidate.y })
		end
	end

	if require_offscreen then
		for _, candidate in pairs(positions) do
			if not self:isOnScreen(candidate, radius, padding) then
				return Vector:new({ x = candidate.x, y = candidate.y })
			end
		end
		return nil
	end

	local fallback = positions[random_int(1, #positions)]
	return Vector:new({ x = fallback.x, y = fallback.y })
end

WorldQuerySampling.resolveRule = function(self, rule, require_offscreen, radius)
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

return WorldQuerySampling
