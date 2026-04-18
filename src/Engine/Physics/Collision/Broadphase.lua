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
	self.contact_pairs = self.contact_pairs or {}
	self.contact_cache_rows = self.contact_cache_rows or {}
	self.contact_stamp = self.contact_stamp or 0
	return setmetatable(self, { __index = CollisionBroadphase })
end

local function entity_has_contact_interest(entity)
	if not entity then
		return false
	end
	local is_sleeping = entity.isPhysicsSleeping and entity:isPhysicsSleeping() or false
	if not is_sleeping then
		local collider = entity.getCollider and entity:getCollider() or nil
		if collider and collider.resolve_dynamic == true and not collider:isTriggerCollider() then
			return true
		end
	end
	if entity.isTrigger and entity:isTrigger() then
		return true
	end
	if entity.hasAnyCollisionInterest and entity:hasAnyCollisionInterest() then
		return true
	end
	if entity.hasAnyOverlapInterest and entity:hasAnyOverlapInterest() then
		return true
	end
	return false
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
	self.contact_stamp += 1
	clear_array(self.active_buckets)
	clear_array(self.contact_pairs)
end

local function build_pair_key(entity_a, entity_b)
	local id_a = entity_a and entity_a.object_id or 0
	local id_b = entity_b and entity_b.object_id or 0
	if id_a > id_b then
		id_a, id_b = id_b, id_a
	end
	return "p:" .. tostr(id_a) .. ":" .. tostr(id_b)
end

local function get_pair_ids(entity_a, entity_b)
	local id_a = entity_a and entity_a.object_id or 0
	local id_b = entity_b and entity_b.object_id or 0
	if id_a > id_b then
		id_a, id_b = id_b, id_a
	end
	return id_a, id_b
end

local function get_contact_cache_pair(self, entity_a, entity_b)
	local id_a, id_b = get_pair_ids(entity_a, entity_b)
	local row = self.contact_cache_rows[id_a]
	if not row then
		row = {}
		self.contact_cache_rows[id_a] = row
	end
	return row, row[id_b], id_a, id_b
end

local function reset_pair_impulses(pair)
	if not pair then
		return
	end
	pair.normal_impulse = 0
	pair.tangent_impulse = 0
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
	local id_a = entity_a and entity_a.object_id or 0
	local id_b = entity_b and entity_b.object_id or 0
	if id_a > id_b then
		entity_a, entity_b = entity_b, entity_a
		push = {
			x = -(push and push.x or 0),
			y = -(push and push.y or 0),
			penetration = push and push.penetration or 0,
			normal_x = -(push and push.normal_x or 0),
			normal_y = -(push and push.normal_y or 0),
		}
	end
	local collider_a = entity_a.getCollider and entity_a:getCollider() or nil
	local collider_b = entity_b.getCollider and entity_b:getCollider() or nil
	local should_resolve = collider_a and collider_b and collider_a:canResolveWith(collider_b) or false
	local collision_interest = (entity_a.hasAnyCollisionInterest and entity_a:hasAnyCollisionInterest() or false) or
		(entity_b.hasAnyCollisionInterest and entity_b:hasAnyCollisionInterest() or false)
	local overlap_interest = (entity_a.hasAnyOverlapInterest and entity_a:hasAnyOverlapInterest() or false) or
		(entity_b.hasAnyOverlapInterest and entity_b:hasAnyOverlapInterest() or false)

	if should_resolve then
		local row, pair, id_a, id_b = get_contact_cache_pair(self, entity_a, entity_b)
		if not pair then
			pair = {
				key = "p:" .. tostr(id_a) .. ":" .. tostr(id_b),
				normal_impulse = 0,
				tangent_impulse = 0,
				persisted_frames = 0,
			}
			row[id_b] = pair
		end
		local previous_stamp = pair.stamp or -999999
		local previous_normal_x = pair.normal_x or 0
		local previous_normal_y = pair.normal_y or 0
		local next_normal_x = push.normal_x or 0
		local next_normal_y = push.normal_y or 0
		local normal_dot = previous_normal_x * next_normal_x + previous_normal_y * next_normal_y
		local contact_persisted = previous_stamp == (self.contact_stamp - 1)
		if not contact_persisted or normal_dot < 0.85 then
			reset_pair_impulses(pair)
			pair.persisted_frames = 0
		else
			pair.persisted_frames = (pair.persisted_frames or 0) + 1
		end
		pair.stamp = self.contact_stamp
		pair.entity_a = entity_a
		pair.entity_b = entity_b
		pair.normal_x = next_normal_x
		pair.normal_y = next_normal_y
		pair.penetration = push.penetration or 0
		pair.contact_x = push.contact_x or ((entity_a.pos.x + entity_b.pos.x) * 0.5)
		pair.contact_y = push.contact_y or ((entity_a.pos.y + entity_b.pos.y) * 0.5)
		add(entity_a.collisions, {
			object = entity_b,
			vector = { x = push.x * 0.5, y = push.y * 0.5 },
			normal = { x = push.normal_x or 0, y = push.normal_y or 0 },
			penetration = push.penetration or 0,
			key = pair.key,
		})
		add(entity_b.collisions, {
			object = entity_a,
			vector = { x = -push.x * 0.5, y = -push.y * 0.5 },
			normal = { x = -(push.normal_x or 0), y = -(push.normal_y or 0) },
			penetration = push.penetration or 0,
			key = pair.key,
		})
		add(self.contact_pairs, pair)
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
			local entity_b = entities[entity_b_index]
			if entity_a.isPhysicsSleeping and entity_b.isPhysicsSleeping and entity_a:isPhysicsSleeping() and entity_b:isPhysicsSleeping() then
				goto continue_b
			end
			if not (entity_a._collision_contact_active or entity_b._collision_contact_active) then
				goto continue_b
			end
			if profiler then
				profiler:addCounter("collision.broadphase.pairs_considered", 1)
			end

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

CollisionBroadphase.collectBucketPairsFromEntities = function(self, bucket)
	local profiler = self.profiler
	if profiler then
		profiler:observe("collision.broadphase.bucket_size", #bucket)
	end
	for i = 1, #bucket - 1 do
		local entity_a = bucket[i]
		if entity_a.ignore_collisions then
			goto continue_a
		end
		local entity_a_index = entity_a._broadphase_pair_id or i

		for j = i + 1, #bucket do
			local entity_b = bucket[j]
			local entity_b_index = entity_b._broadphase_pair_id or j
			if has_visited_pair(self, entity_a_index, entity_b_index) then
				goto continue_b
			end
			if entity_a.isPhysicsSleeping and entity_b.isPhysicsSleeping and entity_a:isPhysicsSleeping() and entity_b:isPhysicsSleeping() then
				goto continue_b
			end
			if not (entity_a._collision_contact_active or entity_b._collision_contact_active) then
				goto continue_b
			end
			if profiler then
				profiler:addCounter("collision.broadphase.pairs_considered", 1)
			end

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

CollisionBroadphase.collectCrossBucketPairsFromEntities = function(self, dynamic_bucket, static_bucket)
	if not dynamic_bucket or not static_bucket then
		return
	end
	local profiler = self.profiler
	if profiler then
		profiler:observe("collision.broadphase.static_bucket_size", #static_bucket)
	end
	for i = 1, #dynamic_bucket do
		local entity_a = dynamic_bucket[i]
		if entity_a.ignore_collisions then
			goto continue_a
		end
		local entity_a_index = entity_a._broadphase_pair_id or i

		for j = 1, #static_bucket do
			local entity_b = static_bucket[j]
			local entity_b_index = entity_b._broadphase_pair_id or j
			if has_visited_pair(self, entity_a_index, entity_b_index) then
				goto continue_b
			end
			if entity_a.isPhysicsSleeping and entity_b.isPhysicsSleeping and entity_a:isPhysicsSleeping() and entity_b:isPhysicsSleeping() then
				goto continue_b
			end
			if not (entity_a._collision_contact_active or entity_b._collision_contact_active) then
				goto continue_b
			end
			if profiler then
				profiler:addCounter("collision.broadphase.pairs_considered", 1)
			end

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
	if self.dynamic_spatial_index and self.static_spatial_index then
		for index = 1, #entities do
			entities[index]._broadphase_pair_id = index
		end
		if profiler then
			profiler:observe("collision.broadphase.bucket_count", #self.dynamic_spatial_index.active_buckets)
			profiler:observe("collision.broadphase.entities", #entities)
		end
		for i = 1, #self.dynamic_spatial_index.active_buckets do
			local bucket = self.dynamic_spatial_index.active_buckets[i]
			if bucket and bucket.stamp == self.dynamic_spatial_index.build_stamp then
				if #bucket > 1 then
					self:collectBucketPairsFromEntities(bucket)
				end
				local static_row = self.static_spatial_index.bucket_rows[bucket.cell_y]
				local static_bucket = static_row and static_row[bucket.cell_x] or nil
				if static_bucket and static_bucket.stamp == self.static_spatial_index.build_stamp and #static_bucket > 0 then
					self:collectCrossBucketPairsFromEntities(bucket, static_bucket)
				end
			end
		end
		return
	end
	if self.spatial_index and self.spatial_index.active_buckets then
		for index = 1, #entities do
			entities[index]._broadphase_pair_id = index
		end
		if profiler then
			profiler:observe("collision.broadphase.bucket_count", #self.spatial_index.active_buckets)
			profiler:observe("collision.broadphase.entities", #entities)
		end
		for i = 1, #self.spatial_index.active_buckets do
			local bucket = self.spatial_index.active_buckets[i]
			if bucket and bucket.stamp == self.spatial_index.build_stamp and #bucket > 1 then
				self:collectBucketPairsFromEntities(bucket)
			end
		end
		return
	end

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
		entity._collision_contact_active = entity_has_contact_interest(entity)
		local is_sleeping = entity.isPhysicsSleeping and entity:isPhysicsSleeping() or false

		if entity.ignore_physics or entity.ignore_collisions then
			goto skip
		end

		if (entity._moved_this_frame or entity._collision_contact_active) and not is_sleeping then
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
		end

		::skip::
	end

	self:collectEntityCollisions(entities)
	for id_a, row in pairs(self.contact_cache_rows or {}) do
		local row_empty = true
		for id_b, pair in pairs(row) do
			if not pair or pair.stamp ~= self.contact_stamp then
				row[id_b] = nil
			else
				row_empty = false
			end
		end
		if row_empty then
			self.contact_cache_rows[id_a] = nil
		end
	end
end

return CollisionBroadphase
