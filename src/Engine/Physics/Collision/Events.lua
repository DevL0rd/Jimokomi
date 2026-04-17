local CollisionEvents = {}

CollisionEvents.handleCollisionEvents = function(self, entities)
	for index = 1, #entities do
		local entity = entities[index]
		if entity.collisions and #entity.collisions > 0 and entity.on_collision then
			for collision_index = 1, #entity.collisions do
				local collision = entity.collisions[collision_index]
				entity:on_collision(collision.object, collision.vector)
			end
		end
		if entity.overlaps and #entity.overlaps > 0 and entity.on_overlap then
			for overlap_index = 1, #entity.overlaps do
				local overlap = entity.overlaps[overlap_index]
				entity:on_overlap(overlap.object, overlap.vector)
			end
		end
	end
end

return CollisionEvents
