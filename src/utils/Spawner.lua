local Vector = include("src/classes/Vector.lua")

local Spawner = {
	getMapBounds = function(self, layer, margin)
		margin = margin or 0
		return {
			left = margin,
			top = margin,
			right = layer.w - margin,
			bottom = layer.h - margin,
		}
	end,

	isOnScreen = function(self, layer, pos, radius, padding)
		padding = padding or 0
		return layer.camera:isRectVisible(
			pos.x - radius - padding,
			pos.y - radius - padding,
			(radius + padding) * 2,
			(radius + padding) * 2
		)
	end,

	getRandomMapPosition = function(self, layer, margin)
		local bounds = self:getMapBounds(layer, margin)
		return Vector:new({
			x = random_int(bounds.left, bounds.right),
			y = random_int(bounds.top, bounds.bottom)
		})
	end,

	collectTilePositions = function(self, layer, tile_ids, offset_x, offset_y)
		offset_x = offset_x or 0
		offset_y = offset_y or 0

		local tile_lookup = {}
		for _, tile_id in pairs(tile_ids) do
			tile_lookup[tile_id] = true
		end

		local positions = {}
		local tile_size = layer.tile_size or 16
		local tiles_w = flr(layer.w / tile_size)
		local tiles_h = flr(layer.h / tile_size)

		for tile_y = 0, tiles_h - 1 do
			for tile_x = 0, tiles_w - 1 do
				local tile_id = mget(tile_x, tile_y)
				if tile_lookup[tile_id] then
					add(positions, Vector:new({
						x = tile_x * tile_size + tile_size / 2 + offset_x,
						y = tile_y * tile_size + tile_size / 2 + offset_y
					}))
				end
			end
		end

		return positions
	end,

	findSurfacePositionAtX = function(self, layer, x, radius, margin)
		margin = margin or 0
		local collision = layer.collision
		local tile_size = layer.tile_size or 16
		local bounds = self:getMapBounds(layer, margin)
		local start_tile_y = max(1, flr(bounds.top / tile_size))
		local end_tile_y = flr(bounds.bottom / tile_size)

		for tile_y = start_tile_y, end_tile_y do
			local tile_center_y = tile_y * tile_size
			local tile_id = collision:getTileIDAt(x, tile_center_y, layer.map_id)

			if collision:isSolidTile(tile_id) then
				local above_tile_id = collision:getTileIDAt(x, (tile_y - 1) * tile_size, layer.map_id)
				if not collision:isSolidTile(above_tile_id) then
					return Vector:new({
						x = x,
						y = tile_y * tile_size - radius
					})
				end
			end
		end

		return nil
	end,

	getRandomSurfacePosition = function(self, layer, radius, margin, require_offscreen, offscreen_padding, max_attempts)
		local bounds = self:getMapBounds(layer, margin)
		max_attempts = max_attempts or 48

		for i = 1, max_attempts do
			local x = random_int(bounds.left, bounds.right)
			local candidate = self:findSurfacePositionAtX(layer, x, radius, margin)
			if candidate and (not require_offscreen or not self:isOnScreen(layer, candidate, radius, offscreen_padding)) then
				return candidate
			end
		end

		for x = bounds.left, bounds.right, layer.tile_size or 16 do
			local candidate = self:findSurfacePositionAtX(layer, x, radius, margin)
			if candidate and (not require_offscreen or not self:isOnScreen(layer, candidate, radius, offscreen_padding)) then
				return candidate
			end
		end

		return nil
	end,

	getRandomTilePosition = function(self, layer, tile_ids, offset_x, offset_y, require_offscreen, radius, padding, max_attempts)
		local positions = self:collectTilePositions(layer, tile_ids, offset_x, offset_y)
		if #positions == 0 then
			return nil
		end

		max_attempts = max_attempts or 24

		for i = 1, max_attempts do
			local candidate = positions[random_int(1, #positions)]
			if not require_offscreen or not self:isOnScreen(layer, candidate, radius or 0, padding or 0) then
				return Vector:new({ x = candidate.x, y = candidate.y })
			end
		end

		local candidate = positions[random_int(1, #positions)]
		return Vector:new({ x = candidate.x, y = candidate.y })
	end,
}

return Spawner
