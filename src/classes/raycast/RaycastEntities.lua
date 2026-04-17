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
	local closest_hit_obj = nil
	local min_t = ray.length

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

	for _, obj in pairs(self.layer.entities) do
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

	if closest_hit_obj ~= nil then
		return closest_hit_obj, min_t
	end

	return nil, ray.length
end

return RaycastEntities
