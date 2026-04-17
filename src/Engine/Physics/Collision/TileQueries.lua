local Vector = include("src/Engine/Math/Vector.lua")

local CollisionTileQueries = {}

CollisionTileQueries.new = function(config)
	local self = config or {}
	self.tile_size = self.tile_size or 16
	self.layer_bounds = self.layer_bounds or { x = 0, y = 0, w = 1024, h = 512 }
	self.tile_properties = self.tile_properties or {}
	return setmetatable(self, { __index = CollisionTileQueries })
end

CollisionTileQueries.addTileType = function(self, tile_id, properties)
	self.tile_properties[tile_id] = properties
end

CollisionTileQueries.getTileIDAt = function(self, x, y, map_id)
	if map_id == nil then
		map_id = 0
	end

	local tile_x = flr(x / self.tile_size)
	local tile_y = flr(y / self.tile_size)
	local tile_id = mget(tile_x, tile_y)
	if tile_id == nil then
		return 0
	end
	return tile_id
end

CollisionTileQueries.getTileProperties = function(self, tile_id)
	return self.tile_registry:get(tile_id)
end

CollisionTileQueries.isSolidTile = function(self, tile_id)
	if tile_id == nil then
		return false
	end
	local properties = self:getTileProperties(tile_id)
	return properties and properties.solid or false
end

CollisionTileQueries.setLayerBounds = function(self, x, y, w, h)
	self.layer_bounds = { x = x, y = y, w = w, h = h }
end

CollisionTileQueries.circleToLayer = function(self, entity)
	local bounds = self.layer_bounds
	local push_x = 0
	local push_y = 0
	local radius = entity:getRadius()

	if entity.pos.x - radius < bounds.x then
		push_x = (bounds.x + radius) - entity.pos.x
	elseif entity.pos.x + radius > bounds.x + bounds.w then
		push_x = (bounds.x + bounds.w) - (entity.pos.x + radius)
	end

	if entity.pos.y - radius < bounds.y then
		push_y = (bounds.y + radius) - entity.pos.y
	elseif entity.pos.y + radius > bounds.y + bounds.h then
		push_y = (bounds.y + bounds.h) - (entity.pos.y + radius)
	end

	if push_x ~= 0 or push_y ~= 0 then
		return Vector:new({ x = push_x, y = push_y })
	end
	return false
end

CollisionTileQueries.rectToLayer = function(self, entity)
	local bounds = self.layer_bounds
	local push_x = 0
	local push_y = 0
	local half_width = entity:getHalfWidth()
	local half_height = entity:getHalfHeight()
	local left = entity.pos.x - half_width
	local right = entity.pos.x + half_width
	local top = entity.pos.y - half_height
	local bottom = entity.pos.y + half_height

	if left < bounds.x then
		push_x = bounds.x - left
	elseif right > bounds.x + bounds.w then
		push_x = (bounds.x + bounds.w) - right
	end

	if top < bounds.y then
		push_y = bounds.y - top
	elseif bottom > bounds.y + bounds.h then
		push_y = (bounds.y + bounds.h) - bottom
	end

	if push_x ~= 0 or push_y ~= 0 then
		return Vector:new({ x = push_x, y = push_y })
	end
	return false
end

CollisionTileQueries.checkMapCollision = function(self, entity, map_id)
	if map_id == nil then
		return false
	end

	local push_x = 0
	local push_y = 0

	if entity.isRectShape and entity:isRectShape() then
		local left = entity.pos.x - entity:getHalfWidth()
		local right = entity.pos.x + entity:getHalfWidth()
		local top = entity.pos.y - entity:getHalfHeight()
		local bottom = entity.pos.y + entity:getHalfHeight()
		local left_tile = flr(left / self.tile_size)
		local right_tile = flr(right / self.tile_size)
		local top_tile = flr(top / self.tile_size)
		local bottom_tile = flr(bottom / self.tile_size)

		for tile_x = left_tile, right_tile do
			for tile_y = top_tile, bottom_tile do
				local tile_id = self:getTileIDAt(tile_x * self.tile_size, tile_y * self.tile_size, map_id)
				if self:isSolidTile(tile_id) then
					local tile_left = tile_x * self.tile_size
					local tile_right = (tile_x + 1) * self.tile_size
					local tile_top = tile_y * self.tile_size
					local tile_bottom = (tile_y + 1) * self.tile_size
					local overlap_left = tile_right - left
					local overlap_right = right - tile_left
					local overlap_top = tile_bottom - top
					local overlap_bottom = bottom - tile_top

					if overlap_left > 0 and overlap_right > 0 and overlap_top > 0 and overlap_bottom > 0 then
						local min_overlap = min(min(overlap_left, overlap_right), min(overlap_top, overlap_bottom))
						local separation = 0.5

						if min_overlap == overlap_left then
							push_x = max(push_x, overlap_left + separation)
						elseif min_overlap == overlap_right then
							push_x = min(push_x, -(overlap_right + separation))
						elseif min_overlap == overlap_top then
							push_y = max(push_y, overlap_top + separation)
						else
							push_y = min(push_y, -(overlap_bottom + separation))
						end
					end
				end
			end
		end
	elseif entity.isCircleShape and entity:isCircleShape() then
		local center_tile_x = flr(entity.pos.x / self.tile_size)
		local center_tile_y = flr(entity.pos.y / self.tile_size)
		local radius = entity:getRadius()
		local radius_in_tiles = ceil(radius / self.tile_size)

		for dx = -radius_in_tiles, radius_in_tiles do
			for dy = -radius_in_tiles, radius_in_tiles do
				local tile_x = center_tile_x + dx
				local tile_y = center_tile_y + dy
				local tile_id = self:getTileIDAt(tile_x * self.tile_size, tile_y * self.tile_size, map_id)

				if self:isSolidTile(tile_id) then
					local tile_left = tile_x * self.tile_size
					local tile_right = (tile_x + 1) * self.tile_size
					local tile_top = tile_y * self.tile_size
					local tile_bottom = (tile_y + 1) * self.tile_size
					local closest_x = max(tile_left, min(entity.pos.x, tile_right))
					local closest_y = max(tile_top, min(entity.pos.y, tile_bottom))
					local delta_x = entity.pos.x - closest_x
					local delta_y = entity.pos.y - closest_y
					local distance = sqrt(delta_x * delta_x + delta_y * delta_y)

					if distance < radius then
						local push_distance = radius - distance
						if distance > 0 then
							push_x = push_x + (delta_x / distance) * push_distance
							push_y = push_y + (delta_y / distance) * push_distance
						else
							local push_left = entity.pos.x - tile_left
							local push_right = tile_right - entity.pos.x
							local push_up = entity.pos.y - tile_top
							local push_down = tile_bottom - entity.pos.y
							local min_push = min(min(push_left, push_right), min(push_up, push_down))

							if min_push == push_left then
								push_x = push_x - push_distance
							elseif min_push == push_right then
								push_x = push_x + push_distance
							elseif min_push == push_up then
								push_y = push_y - push_distance
							else
								push_y = push_y + push_distance
							end
						end
					end
				end
			end
		end
	end

	if push_x ~= 0 or push_y ~= 0 then
		return Vector:new({ x = push_x, y = push_y })
	end
	return false
end

return CollisionTileQueries
