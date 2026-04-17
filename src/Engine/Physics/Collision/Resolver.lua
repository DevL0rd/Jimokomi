local CollisionResolver = {}

CollisionResolver.new = function(config)
	local self = config or {}
	self.collision_passes = self.collision_passes or 1
	self.wall_friction = self.wall_friction or 0.8
	return setmetatable(self, { __index = CollisionResolver })
end

CollisionResolver.processCollisions = function(self, entities)
	local profiler = self.profiler
	for index = 1, #entities do
		local entity = entities[index]
		if entity.ignore_physics or entity.ignore_collisions or not entity.collisions then
			goto skip
		end

		local has_x_collision = false
		local has_y_collision = false

		for pass = 1, 3 do
			for collision_index = 1, #entity.collisions do
				local collision = entity.collisions[collision_index]
				local object = collision.object
				if (pass == 1 and object == "map") or
					(pass == 2 and object == "layer") or
					(pass == 3 and object ~= "map" and object ~= "layer") then
					entity.pos:add(collision.vector)
					if profiler then
						profiler:addCounter("collision.resolver.contacts_processed", 1)
					end

					if collision.vector.x ~= 0 then
						has_x_collision = true
					end
					if collision.vector.y ~= 0 then
						has_y_collision = true
					end
				end
			end
		end

		if entity.vel and (has_x_collision or has_y_collision) then
			entity.vel:drag(self.wall_friction, true)
			if has_y_collision then
				entity.vel.y = 0
			elseif has_x_collision then
				entity.vel.x = 0
			end
		end

		::skip::
	end
end

CollisionResolver.resolveCollisions = function(self, entities, map_id, broadphase, tile_queries)
	for _ = 1, self.collision_passes do
		broadphase:collectCollisions(entities, map_id, tile_queries)
		self:processCollisions(entities)
	end
end

return CollisionResolver
