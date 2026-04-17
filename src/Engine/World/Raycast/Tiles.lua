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
	local tile_x = flr(start_x / tile_size)
	local tile_y = flr(start_y / tile_size)
	local step_x = dir_x > 0 and 1 or (dir_x < 0 and -1 or 0)
	local step_y = dir_y > 0 and 1 or (dir_y < 0 and -1 or 0)
	local inv_dir_x = dir_x ~= 0 and (1 / dir_x) or 0
	local inv_dir_y = dir_y ~= 0 and (1 / dir_y) or 0
	local delta_dist_x = dir_x ~= 0 and abs(tile_size * inv_dir_x) or 32767
	local delta_dist_y = dir_y ~= 0 and abs(tile_size * inv_dir_y) or 32767
	local next_boundary_x = step_x > 0 and ((tile_x + 1) * tile_size) or (tile_x * tile_size)
	local next_boundary_y = step_y > 0 and ((tile_y + 1) * tile_size) or (tile_y * tile_size)
	local side_dist_x = dir_x ~= 0 and abs((next_boundary_x - start_x) * inv_dir_x) or 32767
	local side_dist_y = dir_y ~= 0 and abs((next_boundary_y - start_y) * inv_dir_y) or 32767
	local current_t = 0

	while true do
		local tile_id = collision_system:getTileIDAt(tile_x * tile_size, tile_y * tile_size, map_id)
		if collision_system:isSolidTile(tile_id) then
			if current_t <= ray.length then
				return max(0, current_t)
			end
			return false
		end

		if side_dist_x < side_dist_y then
			if side_dist_x > ray.length then
				break
			end
			current_t = side_dist_x
			tile_x += step_x
			side_dist_x += delta_dist_x
		else
			if side_dist_y > ray.length then
				break
			end
			current_t = side_dist_y
			tile_y += step_y
			side_dist_y += delta_dist_y
		end
	end

	return false
end

return RaycastTiles
