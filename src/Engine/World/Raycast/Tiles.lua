local RaycastTiles = {}

RaycastTiles.rayToTiles = function(self, ray, map_id, tile_size)
	if not map_id or not self.layer or not self.layer.collision then
		return false
	end

	tile_size = tile_size or 16
	local collision_system = self.layer.collision
	local start_x = ray.pos.x
	local start_y = ray.pos.y
	local dir_x = ray.vec.x
	local dir_y = ray.vec.y
	local length = sqrt(dir_x * dir_x + dir_y * dir_y)

	if length == 0 then
		return false
	end

	dir_x = dir_x / length
	dir_y = dir_y / length

	local current_x = start_x
	local current_y = start_y
	local step_size = tile_size / 4
	local distance = 0

	while distance < ray.length do
		local tile_x = flr(current_x / tile_size)
		local tile_y = flr(current_y / tile_size)
		local tile_id = collision_system:getTileIDAt(tile_x * tile_size, tile_y * tile_size, map_id)

		if collision_system:isSolidTile(tile_id) then
			local tile_left = tile_x * tile_size
			local tile_right = (tile_x + 1) * tile_size
			local tile_top = tile_y * tile_size
			local tile_bottom = (tile_y + 1) * tile_size
			local intersections = {}

			if dir_x ~= 0 then
				local left_t = (tile_left - start_x) / dir_x
				local left_y = start_y + left_t * dir_y
				if left_t >= 0 and left_y >= tile_top and left_y <= tile_bottom then
					add(intersections, left_t)
				end

				local right_t = (tile_right - start_x) / dir_x
				local right_y = start_y + right_t * dir_y
				if right_t >= 0 and right_y >= tile_top and right_y <= tile_bottom then
					add(intersections, right_t)
				end
			end

			if dir_y ~= 0 then
				local top_t = (tile_top - start_y) / dir_y
				local top_x = start_x + top_t * dir_x
				if top_t >= 0 and top_x >= tile_left and top_x <= tile_right then
					add(intersections, top_t)
				end

				local bottom_t = (tile_bottom - start_y) / dir_y
				local bottom_x = start_x + bottom_t * dir_x
				if bottom_t >= 0 and bottom_x >= tile_left and bottom_x <= tile_right then
					add(intersections, bottom_t)
				end
			end

			local min_t = ray.length
			for _, t in pairs(intersections) do
				if t >= 0 and t < min_t then
					min_t = t
				end
			end

			if min_t < ray.length then
				return min_t
			end
		end

		current_x = current_x + dir_x * step_size
		current_y = current_y + dir_y * step_size
		distance = distance + step_size
	end

	return false
end

return RaycastTiles
