local Vector = include("src/Engine/Math/Vector.lua")

local WorldTiles = {}

WorldTiles.getTileAt = function(self, x, y)
	if not self.layer or not self.layer.collision then
		return 0
	end
	return self.layer.collision:getTileIDAt(x, y, self.layer.map_id)
end

WorldTiles.isSolidAt = function(self, x, y)
	if not self.layer or not self.layer.collision then
		return false
	end
	return self.layer.collision:isSolidTile(self:getTileAt(x, y))
end

WorldTiles.collectTilePositions = function(self, tile_ids, rule)
	rule = rule or {}
	local offset = self:getOffset(rule)
	local tile_lookup = {}
	for _, tile_id in pairs(tile_ids or {}) do
		tile_lookup[tile_id] = true
	end

	local positions = {}
	local tile_size = self.layer.tile_size or 16
	local bounds = self:getQueryBounds(rule)
	local start_tile_x = max(0, flr(bounds.left / tile_size))
	local end_tile_x = max(start_tile_x, flr(bounds.right / tile_size))
	local start_tile_y = max(0, flr(bounds.top / tile_size))
	local end_tile_y = max(start_tile_y, flr(bounds.bottom / tile_size))

	for tile_y = start_tile_y, end_tile_y do
		for tile_x = start_tile_x, end_tile_x do
			local tile_id = self.layer.collision:getTileIDAt(tile_x * tile_size, tile_y * tile_size, self.layer.map_id)
			if tile_lookup[tile_id] then
				add(positions, Vector:new({
					x = tile_x * tile_size + tile_size / 2 + offset.x,
					y = tile_y * tile_size + tile_size / 2 + offset.y
				}))
			end
		end
	end

	return positions
end

WorldTiles.findSurfacePositionAtX = function(self, x, radius, rule)
	rule = rule or {}
	local collision = self.layer.collision
	local tile_size = self.layer.tile_size or 16
	local bounds = self:getQueryBounds(rule)
	local start_tile_y = max(1, flr(bounds.top / tile_size))
	local end_tile_y = flr(bounds.bottom / tile_size)

	for tile_y = start_tile_y, end_tile_y do
		local tile_center_y = tile_y * tile_size
		local tile_id = collision:getTileIDAt(x, tile_center_y, self.layer.map_id)

		if collision:isSolidTile(tile_id) then
			local above_tile_id = collision:getTileIDAt(x, (tile_y - 1) * tile_size, self.layer.map_id)
			if not collision:isSolidTile(above_tile_id) then
				return Vector:new({
					x = x,
					y = tile_y * tile_size - radius
				})
			end
		end
	end

	return nil
end

return WorldTiles
