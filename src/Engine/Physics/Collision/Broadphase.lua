local Vector = include("src/Engine/Math/Vector.lua")

local CollisionBroadphase = {}

CollisionBroadphase.new = function(config)
	local self = config or {}
	self.cell_size = self.cell_size or 32
	return setmetatable(self, { __index = CollisionBroadphase })
end

CollisionBroadphase.canEntitiesInteract = function(self, entity_a, entity_b)
	if entity_a == entity_b then
		return false
	end
	if entity_a.ignore_collisions or entity_b.ignore_collisions then
		return false
	end
	if entity_a.getCollider and entity_a:getCollider() and not entity_a:getCollider():isEnabled() then
		return false
	end
	if entity_b.getCollider and entity_b:getCollider() and not entity_b:getCollider():isEnabled() then
		return false
	end
	if entity_a.canCollideWith and not entity_a:canCollideWith(entity_b) then
		return false
	end
	if entity_b.canCollideWith and not entity_b:canCollideWith(entity_a) then
		return false
	end
	return true
end

CollisionBroadphase.getEntityBounds = function(self, entity)
	local half_width = entity.isCircleShape and entity:isCircleShape() and entity:getRadius() or entity:getHalfWidth()
	local half_height = entity.isCircleShape and entity:isCircleShape() and entity:getRadius() or entity:getHalfHeight()
	return {
		left = entity.pos.x - half_width,
		right = entity.pos.x + half_width,
		top = entity.pos.y - half_height,
		bottom = entity.pos.y + half_height,
	}
end

CollisionBroadphase.addEntityToBuckets = function(self, buckets, entity, index)
	local bounds = self:getEntityBounds(entity)
	local cell_size = self.cell_size or 32
	local left = flr(bounds.left / cell_size)
	local right = flr(bounds.right / cell_size)
	local top = flr(bounds.top / cell_size)
	local bottom = flr(bounds.bottom / cell_size)

	for cell_y = top, bottom do
		for cell_x = left, right do
			local key = cell_x .. ":" .. cell_y
			if not buckets[key] then
				buckets[key] = {}
			end
			add(buckets[key], index)
		end
	end
end

CollisionBroadphase.recordEntityInteraction = function(self, entity_a, entity_b, push)
	local collider_a = entity_a.getCollider and entity_a:getCollider() or nil
	local collider_b = entity_b.getCollider and entity_b:getCollider() or nil
	local should_resolve = collider_a and collider_b and collider_a:canResolveWith(collider_b) or false

	if should_resolve then
		add(entity_a.collisions, {
			object = entity_b,
			vector = Vector:new({ x = push.x * 0.5, y = push.y * 0.5 }),
		})
		add(entity_b.collisions, {
			object = entity_a,
			vector = Vector:new({ x = -push.x * 0.5, y = -push.y * 0.5 }),
		})
		return
	end

	local is_trigger = entity_a:isTrigger() or entity_b:isTrigger()
	add(entity_a.overlaps, {
		object = entity_b,
		vector = Vector:new({ x = push.x, y = push.y }),
		trigger = is_trigger,
	})
	add(entity_b.overlaps, {
		object = entity_a,
		vector = Vector:new({ x = -push.x, y = -push.y }),
		trigger = is_trigger,
	})
end

CollisionBroadphase.collectBucketPairs = function(self, entities, bucket, visited)
	for i = 1, #bucket - 1 do
		local entity_a_index = bucket[i]
		local entity_a = entities[entity_a_index]
		if entity_a.ignore_collisions then
			goto continue_a
		end

		for j = i + 1, #bucket do
			local entity_b_index = bucket[j]
			local pair_key
			if entity_a_index < entity_b_index then
				pair_key = entity_a_index .. ":" .. entity_b_index
			else
				pair_key = entity_b_index .. ":" .. entity_a_index
			end

			if visited[pair_key] then
				goto continue_b
			end
			visited[pair_key] = true

			local entity_b = entities[entity_b_index]
			if not self:canEntitiesInteract(entity_a, entity_b) then
				goto continue_b
			end
			if not self.shape_tests:boundsOverlap(entity_a, entity_b) then
				goto continue_b
			end

			local push = self.shape_tests:entityToEntity(entity_a, entity_b)
			if not push then
				goto continue_b
			end

			self:recordEntityInteraction(entity_a, entity_b, push)

			::continue_b::
		end

		::continue_a::
	end
end

CollisionBroadphase.collectEntityCollisions = function(self, entities)
	local buckets = {}
	local visited = {}

	for index = 1, #entities do
		local entity = entities[index]
		if not entity.ignore_collisions then
			self:addEntityToBuckets(buckets, entity, index)
		end
	end

	for _, bucket in pairs(buckets) do
		if #bucket > 1 then
			self:collectBucketPairs(entities, bucket, visited)
		end
	end
end

CollisionBroadphase.collectCollisions = function(self, entities, map_id, tile_queries)
	for index = 1, #entities do
		local entity = entities[index]
		entity.collisions = {}
		entity.overlaps = {}

		if entity.ignore_physics or entity.ignore_collisions then
			goto skip
		end

		local layer_collision = nil
		if entity:isRectShape() then
			layer_collision = tile_queries:rectToLayer(entity)
		elseif entity:isCircleShape() then
			layer_collision = tile_queries:circleToLayer(entity)
		end

		if layer_collision then
			add(entity.collisions, {
				object = "layer",
				vector = layer_collision,
			})
		end

		local map_collision = tile_queries:checkMapCollision(entity, map_id)
		if map_collision then
			add(entity.collisions, {
				object = "map",
				vector = map_collision,
			})
		end

		::skip::
	end

	self:collectEntityCollisions(entities)
end

return CollisionBroadphase
