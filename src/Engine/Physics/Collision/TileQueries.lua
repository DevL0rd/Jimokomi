local Vector = include("src/Engine/Math/Vector.lua")

local CollisionTileQueries = {}

CollisionTileQueries.new = function(config)
	local self = config or {}
	self.tile_size = self.tile_size or 16
	self.layer_bounds = self.layer_bounds or { x = 0, y = 0, w = 1024, h = 512 }
	self.tile_properties = self.tile_properties or {}
	self.solid_row_spans_by_map = {}
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
	local bounds = self.layer_bounds or {}
	if bounds.x == x and bounds.y == y and bounds.w == w and bounds.h == h then
		return false
	end
	self.layer_bounds = { x = x, y = y, w = w, h = h }
	self:invalidateSolidRowSpans()
	return true
end

CollisionTileQueries.invalidateSolidRowSpans = function(self, map_id)
	if map_id == nil then
		self.solid_row_spans_by_map = {}
		return
	end
	self.solid_row_spans_by_map[map_id] = nil
end

CollisionTileQueries.buildSolidRowSpans = function(self, map_id)
	local key = map_id or 0
	local row_count = max(1, ceil((self.layer_bounds.h or 0) / self.tile_size))
	local col_count = max(1, ceil((self.layer_bounds.w or 0) / self.tile_size))
	local spans_by_row = {}

	for tile_y = 0, row_count - 1 do
		local spans = {}
		local span_start = nil
		for tile_x = 0, col_count - 1 do
			local tile_id = self:getTileIDAt(tile_x * self.tile_size, tile_y * self.tile_size, key)
			if self:isSolidTile(tile_id) then
				if span_start == nil then
					span_start = tile_x
				end
			elseif span_start ~= nil then
				add(spans, {
					start_x = span_start,
					end_x = tile_x - 1,
				})
				span_start = nil
			end
		end
		if span_start ~= nil then
			add(spans, {
				start_x = span_start,
				end_x = col_count - 1,
			})
		end
		spans_by_row[tile_y] = spans
	end

	self.solid_row_spans_by_map[key] = spans_by_row
	return spans_by_row
end

CollisionTileQueries.getSolidRowSpans = function(self, map_id)
	local key = map_id or 0
	local spans_by_row = self.solid_row_spans_by_map[key]
	if spans_by_row then
		return spans_by_row
	end
	return self:buildSolidRowSpans(key)
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
	local profiler = self.profiler
	local solid_row_spans = self:getSolidRowSpans(map_id)

	if entity.isRectShape and entity:isRectShape() then
		local left = entity.pos.x - entity:getHalfWidth()
		local right = entity.pos.x + entity:getHalfWidth()
		local top = entity.pos.y - entity:getHalfHeight()
		local bottom = entity.pos.y + entity:getHalfHeight()
		local left_tile = flr(left / self.tile_size)
		local right_tile = flr(right / self.tile_size)
		local top_tile = flr(top / self.tile_size)
		local bottom_tile = flr(bottom / self.tile_size)

		for tile_y = top_tile, bottom_tile do
			local row_spans = solid_row_spans[tile_y]
			if profiler then
				profiler:addCounter("collision.tiles.rect_rows", 1)
			end
			if row_spans then
				for i = 1, #row_spans do
					local span = row_spans[i]
					local start_x = max(left_tile, span.start_x)
					local end_x = min(right_tile, span.end_x)
					if end_x >= start_x then
						if profiler then
							profiler:addCounter("collision.tiles.rect_candidate_tiles", end_x - start_x + 1)
						end
						for tile_x = start_x, end_x do
							if profiler then
								profiler:addCounter("collision.tiles.rect_checks", 1)
							end
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
			end
		end
	elseif entity.isCircleShape and entity:isCircleShape() then
		local radius = entity:getRadius()
		local left_tile = flr((entity.pos.x - radius) / self.tile_size)
		local right_tile = flr((entity.pos.x + radius) / self.tile_size)
		local top_tile = flr((entity.pos.y - radius) / self.tile_size)
		local bottom_tile = flr((entity.pos.y + radius) / self.tile_size)
		local radius_sq = radius * radius

		for tile_y = top_tile, bottom_tile do
			local row_spans = solid_row_spans[tile_y]
			if profiler then
				profiler:addCounter("collision.tiles.circle_rows", 1)
			end
			if row_spans then
				for i = 1, #row_spans do
					local span = row_spans[i]
					local start_x = max(left_tile, span.start_x)
					local end_x = min(right_tile, span.end_x)
					if end_x >= start_x then
						if profiler then
							profiler:addCounter("collision.tiles.circle_candidate_tiles", end_x - start_x + 1)
						end
						for tile_x = start_x, end_x do
							if profiler then
								profiler:addCounter("collision.tiles.circle_checks", 1)
							end
							local tile_left = tile_x * self.tile_size
							local tile_right = (tile_x + 1) * self.tile_size
							local tile_top = tile_y * self.tile_size
							local tile_bottom = (tile_y + 1) * self.tile_size
							local closest_x = max(tile_left, min(entity.pos.x, tile_right))
							local closest_y = max(tile_top, min(entity.pos.y, tile_bottom))
							local delta_x = entity.pos.x - closest_x
							local delta_y = entity.pos.y - closest_y
							local distance_sq = delta_x * delta_x + delta_y * delta_y

							if distance_sq < radius_sq then
								local distance = distance_sq > 0 and sqrt(distance_sq) or 0
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
		end
	end

	if push_x ~= 0 or push_y ~= 0 then
		return Vector:new({ x = push_x, y = push_y })
	end
	return false
end

return CollisionTileQueries
