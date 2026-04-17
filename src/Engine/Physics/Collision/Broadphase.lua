local CollisionBroadphase = {}

local function clear_array(values)
	if not values then
		return {}
	end
	for i = #values, 1, -1 do
		values[i] = nil
	end
	return values
end

CollisionBroadphase.new = function(config)
	local self = config or {}
	self.cell_size = self.cell_size or 32
	self.bucket_rows = self.bucket_rows or {}
	self.active_buckets = self.active_buckets or {}
	self.bucket_stamp = self.bucket_stamp or 0
	self.visited_pairs = self.visited_pairs or {}
	self.pair_stamp = self.pair_stamp or 0
	return setmetatable(self, { __index = CollisionBroadphase })
end

local function get_bucket(self, cell_x, cell_y)
	local row = self.bucket_rows[cell_y]
	if not row then
		row = {}
		self.bucket_rows[cell_y] = row
	end
	local bucket = row[cell_x]
	if not bucket then
		bucket = { stamp = 0 }
		row[cell_x] = bucket
	end
	return bucket
end

local function begin_bucket_collection(self)
	self.bucket_stamp += 1
	self.pair_stamp += 1
	clear_array(self.active_buckets)
end

local function has_visited_pair(self, entity_a_index, entity_b_index)
	local low = entity_a_index
	local high = entity_b_index
	if low > high then
		low, high = high, low
	end
	local visited_row = self.visited_pairs[low]
	if not visited_row then
		visited_row = {}
		self.visited_pairs[low] = visited_row
	end
	if visited_row[high] == self.pair_stamp then
		return true
	end
	visited_row[high] = self.pair_stamp
	return false
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
	return
		entity.pos.x - half_width,
		entity.pos.x + half_width,
		entity.pos.y - half_height,
		entity.pos.y + half_height
end

CollisionBroadphase.addEntityToBuckets = function(self, buckets, entity, index)
	local cell_size = self.cell_size or 32
	local left_bound, right_bound, top_bound, bottom_bound = self:getEntityBounds(entity)
	local left = flr(left_bound / cell_size)
	local right = flr(right_bound / cell_size)
	local top = flr(top_bound / cell_size)
	local bottom = flr(bottom_bound / cell_size)

	for cell_y = top, bottom do
		for cell_x = left, right do
			local bucket = get_bucket(self, cell_x, cell_y)
			if bucket.stamp ~= self.bucket_stamp then
				clear_array(bucket)
				bucket.stamp = self.bucket_stamp
				add(self.active_buckets, bucket)
			end
			add(bucket, index)
		end
	end
end

CollisionBroadphase.recordEntityInteraction = function(self, entity_a, entity_b, push)
	local collider_a = entity_a.getCollider and entity_a:getCollider() or nil
	local collider_b = entity_b.getCollider and entity_b:getCollider() or nil
	local should_resolve = collider_a and collider_b and collider_a:canResolveWith(collider_b) or false
	local collision_interest = (entity_a.hasAnyCollisionInterest and entity_a:hasAnyCollisionInterest() or false) or
		(entity_b.hasAnyCollisionInterest and entity_b:hasAnyCollisionInterest() or false)
	local overlap_interest = (entity_a.hasAnyOverlapInterest and entity_a:hasAnyOverlapInterest() or false) or
		(entity_b.hasAnyOverlapInterest and entity_b:hasAnyOverlapInterest() or false)

	if should_resolve then
		add(entity_a.collisions, {
			object = entity_b,
			vector = { x = push.x * 0.5, y = push.y * 0.5 },
		})
		add(entity_b.collisions, {
			object = entity_a,
			vector = { x = -push.x * 0.5, y = -push.y * 0.5 },
		})
		return
	end

	local is_trigger = entity_a:isTrigger() or entity_b:isTrigger()
	if not is_trigger and not overlap_interest and not collision_interest then
		return
	end
	add(entity_a.overlaps, {
		object = entity_b,
		vector = { x = push.x, y = push.y },
		trigger = is_trigger,
	})
	add(entity_b.overlaps, {
		object = entity_a,
		vector = { x = -push.x, y = -push.y },
		trigger = is_trigger,
	})
end

CollisionBroadphase.canEntitiesGenerateContact = function(self, entity_a, entity_b)
	local collider_a = entity_a.getCollider and entity_a:getCollider() or nil
	local collider_b = entity_b.getCollider and entity_b:getCollider() or nil
	local should_resolve = collider_a and collider_b and collider_a:canResolveWith(collider_b) or false
	if should_resolve then
		return true
	end

	local is_trigger = entity_a.isTrigger and entity_a:isTrigger() or entity_b.isTrigger and entity_b:isTrigger() or false
	if is_trigger then
		return true
	end

	local collision_interest = (entity_a.hasAnyCollisionInterest and entity_a:hasAnyCollisionInterest() or false) or
		(entity_b.hasAnyCollisionInterest and entity_b:hasAnyCollisionInterest() or false)
	if collision_interest then
		return true
	end

	local overlap_interest = (entity_a.hasAnyOverlapInterest and entity_a:hasAnyOverlapInterest() or false) or
		(entity_b.hasAnyOverlapInterest and entity_b:hasAnyOverlapInterest() or false)
	return overlap_interest
end

CollisionBroadphase.collectBucketPairs = function(self, entities, bucket)
	local profiler = self.profiler
	if profiler then
		profiler:observe("collision.broadphase.bucket_size", #bucket)
	end
	for i = 1, #bucket - 1 do
		local entity_a_index = bucket[i]
		local entity_a = entities[entity_a_index]
		if entity_a.ignore_collisions then
			goto continue_a
		end

		for j = i + 1, #bucket do
			local entity_b_index = bucket[j]
			if has_visited_pair(self, entity_a_index, entity_b_index) then
				goto continue_b
			end
			if profiler then
				profiler:addCounter("collision.broadphase.pairs_considered", 1)
			end

			local entity_b = entities[entity_b_index]
			if not self:canEntitiesInteract(entity_a, entity_b) then
				goto continue_b
			end
			if not self:canEntitiesGenerateContact(entity_a, entity_b) then
				goto continue_b
			end
			if not self.shape_tests:boundsOverlap(entity_a, entity_b) then
				goto continue_b
			end

			local push = self.shape_tests:entityToEntity(entity_a, entity_b)
			if not push then
				goto continue_b
			end
			if profiler then
				profiler:addCounter("collision.broadphase.pairs_hit", 1)
			end

			self:recordEntityInteraction(entity_a, entity_b, push)

			::continue_b::
		end

		::continue_a::
	end
end

CollisionBroadphase.collectEntityCollisions = function(self, entities)
	local profiler = self.profiler
	begin_bucket_collection(self)

	for index = 1, #entities do
		local entity = entities[index]
		if not entity.ignore_collisions then
			self:addEntityToBuckets(nil, entity, index)
		end
	end
	if profiler then
		profiler:observe("collision.broadphase.bucket_count", #self.active_buckets)
		profiler:observe("collision.broadphase.entities", #entities)
	end

	for i = 1, #self.active_buckets do
		local bucket = self.active_buckets[i]
		if #bucket > 1 then
			self:collectBucketPairs(entities, bucket)
		end
	end
end

CollisionBroadphase.collectCollisions = function(self, entities, map_id, tile_queries)
	for index = 1, #entities do
		local entity = entities[index]
		entity.collisions = clear_array(entity.collisions)
		entity.overlaps = clear_array(entity.overlaps)

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
