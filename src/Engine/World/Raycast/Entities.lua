local RaycastEntities = {}

RaycastEntities.canRayHitEntity = function(self, ray, obj)
	if obj == ray or obj == ray.parent then
		return false
	end
	if obj.ignore_collisions then
		return false
	end
	if obj.getCollider and obj:getCollider() and not obj:getCollider():isEnabled() then
		return false
	end
	return true
end

RaycastEntities.rayToEntity = function(self, ray, obj)
	if obj.isCircleShape and obj:isCircleShape() then
		return self:rayToCircle(ray, obj)
	end
	if obj.isRectShape and obj:isRectShape() then
		return self:rayToRect(ray, obj)
	end
	return false
end

RaycastEntities.cast = function(self, ray)
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	local scope = profiler and profiler:start("world.raycast.cast") or nil
	local closest_hit_obj = nil
	local min_t = ray.length
	local candidates_considered = 0

	local layer_t = self:rayToLayer(ray, { x = 0, y = 0, w = self.layer.w, h = self.layer.h })
	if layer_t ~= false and layer_t >= 0 and layer_t < min_t then
		min_t = layer_t
		closest_hit_obj = "layer"
	end

	if self.layer.map_id then
		local tile_t = self:rayToTiles(ray, self.layer.map_id, self.layer.tile_size or 16)
		if tile_t ~= false and tile_t >= 0 and tile_t < min_t then
			min_t = tile_t
			closest_hit_obj = "tile"
		end
	end

	local targets = self.layer.collidable_entities or self.layer.entities
	for i = 1, #targets do
		local obj = targets[i]
		candidates_considered += 1
		if not self:canRayHitEntity(ray, obj) then
			goto continue
		end

		local t = self:rayToEntity(ray, obj)
		if t ~= false and t >= 0 and t < min_t then
			min_t = t
			closest_hit_obj = obj
		end

		::continue::
	end
	if profiler then
		profiler:observe("world.raycast.candidates", candidates_considered)
		profiler:observe("world.raycast.target_pool", #targets)
	end

	if closest_hit_obj ~= nil then
		if profiler then profiler:stop(scope) end
		return closest_hit_obj, min_t
	end

	if profiler then profiler:stop(scope) end
	return nil, ray.length
end

return RaycastEntities
