local Class = include("src/Engine/Core/Class.lua")
local ENTITY_KIND_PARTICLE = 1
local ENTITY_KIND_PROCEDURAL = 2

local function get_entity_type_key(ent)
	if ent and ent._prof_type_key ~= nil then
		return ent._prof_type_key
	end
	local ent_type = ent and ent._type or nil
	if ent_type == nil or ent_type == "" then
		ent_type = "unknown"
	else
		ent_type = tostr(ent_type)
	end
	if ent then
		ent._prof_type_key = ent_type
	end
	return ent_type
end

local function get_entity_object_key(ent)
	local object_id = ent and ent.object_id or nil
	local cached_key = ent and ent._prof_object_key or nil
	local cached_id = ent and ent._prof_object_id or nil
	if cached_key ~= nil and cached_id == object_id then
		return cached_key
	end
	local object_key = get_entity_type_key(ent) .. "#" .. (object_id == nil and "?" or tostr(object_id))
	if ent then
		ent._prof_object_key = object_key
		ent._prof_object_id = object_id
	end
	return object_key
end

local function get_update_type_scope_name(ent)
	if ent and ent._prof_update_type_scope_name ~= nil then
		return ent._prof_update_type_scope_name
	end
	local scope_name = "layer.sim.update_entities.type." .. get_entity_type_key(ent)
	if ent then
		ent._prof_update_type_scope_name = scope_name
	end
	return scope_name
end

local function get_update_object_scope_name(ent)
	if ent and ent._prof_update_object_scope_name ~= nil and ent._prof_object_id == ent.object_id then
		return ent._prof_update_object_scope_name
	end
	local scope_name = "layer.sim.update_entities.object." .. get_entity_object_key(ent)
	if ent then
		ent._prof_update_object_scope_name = scope_name
	end
	return scope_name
end

local function can_island_sleep_entity(ent)
	return ent and ent.canSleepPhysics and ent:canSleepPhysics() and not ent._delete
end

local function get_sleep_contacts(ent)
	local contacts = {}
	local collisions = ent and ent.collisions or nil
	for i = 1, #(collisions or {}) do
		local other = collisions[i] and collisions[i].object or nil
		if type(other) == "table" and can_island_sleep_entity(other) then
			add(contacts, other)
		end
	end
	return contacts
end

local function island_sleep_candidate(ent)
	if not can_island_sleep_entity(ent) then
		return false
	end
	if ent._wake_physics_requested == true then
		return false
	end
	local vel = ent.vel or {}
	local accel = ent.accel or {}
	local speed_x = abs(vel.x or 0)
	local speed_y = abs(vel.y or 0)
	local accel_x = abs(accel.x or 0)
	local accel_y = abs(accel.y or 0)
	local angular_speed = abs(ent.angular_vel or 0)
	local angular_accel = abs(ent.angular_accel or 0)
	local collision_count = #(ent.collisions or {})
	local sleep_speed = ent.physics_sleep_velocity_threshold or 2.0
	local sleep_accel = ent.physics_sleep_accel_threshold or 0.01
	local sleep_move = ent.physics_sleep_move_threshold or 0.2
	local sleep_angular_speed = ent.physics_sleep_angular_velocity_threshold or 0.5
	local sleep_angular_accel = ent.physics_sleep_angular_accel_threshold or 0.05
	if collision_count > 0 then
		sleep_speed = max(sleep_speed, ent.physics_sleep_contact_velocity_threshold or 4.0)
		sleep_move = 999999
		sleep_angular_speed = max(sleep_angular_speed, ent.physics_sleep_contact_angular_velocity_threshold or 1.0)
	end
	local start_x = ent._physics_step_start_x or ent.pos.x or 0
	local start_y = ent._physics_step_start_y or ent.pos.y or 0
	local end_x = ent.pos and ent.pos.x or start_x
	local end_y = ent.pos and ent.pos.y or start_y
	local moved_x = abs(end_x - start_x)
	local moved_y = abs(end_y - start_y)
	return not (
		speed_x > sleep_speed or speed_y > sleep_speed or
		accel_x > sleep_accel or accel_y > sleep_accel or
		angular_speed > sleep_angular_speed or angular_accel > sleep_angular_accel or
		moved_x > sleep_move or moved_y > sleep_move
	)
end

local function get_required_sleep_frames(ent)
	local collision_count = #(ent and ent.collisions or {})
	if collision_count > 0 then
		return ent.physics_sleep_contact_frames or 6
	end
	return ent.physics_sleep_frames or 10
end

local Simulation = Class:new({
	_type = "Simulation",
	layer = nil,

	syncAttachments = function(self)
		local synced_count = 0
		for i = 1, #self.layer.attached_entities do
			local ent = self.layer.attached_entities[i]
			if ent.parent and ent.getTransform and ent:getTransform() and ent:shouldSyncToParent() then
				ent:syncToParent()
				synced_count += 1
			end
		end
		return synced_count
	end,

	updateEntities = function(self)
		local updated_count = 0
		local removed_count = 0
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local detailed_entity_profiling = self.layer and self.layer.engine and self.layer.engine.detailed_entity_profiling_enabled == true
		for i = #self.layer.entities, 1, -1 do
			local ent = self.layer.entities[i]
			if not ent._delete then
				if ent.needsUpdate == nil or ent:needsUpdate() then
					local type_scope = nil
					local object_scope = nil
					if profiler and detailed_entity_profiling then
						type_scope = profiler:start(get_update_type_scope_name(ent))
						object_scope = profiler:start(get_update_object_scope_name(ent))
					end
					ent:update()
					if profiler then
						profiler:stop(object_scope)
						profiler:stop(type_scope)
					end
					updated_count += 1
				end
			else
				ent:willRemoveFromLayer(self.layer)
				ent:unInit()
				ent:didRemoveFromLayer(self.layer)
				self.layer:unregisterEntityBuckets(ent)
				del(self.layer.entities, ent)
				removed_count += 1
			end
		end
		return updated_count, removed_count
	end,

	applyPhysics = function(self)
		if not self.layer.physics_enabled then
			return 0
		end

		local moved_count = 0
		for i = 1, #self.layer.physics_entities do
			local ent = self.layer.physics_entities[i]
			local prev_x = ent.pos and ent.pos.x or 0
			local prev_y = ent.pos and ent.pos.y or 0
			ent._physics_step_start_x = prev_x
			ent._physics_step_start_y = prev_y
			ent._moved_this_frame = false
			if ent.isPhysicsSleeping and ent:isPhysicsSleeping() then
				local vel = ent.vel or nil
				local accel = ent.accel or nil
				local speed_x = abs(vel and vel.x or 0)
				local speed_y = abs(vel and vel.y or 0)
				local accel_x = abs(accel and accel.x or 0)
				local accel_y = abs(accel and accel.y or 0)
				local angular_speed = abs(ent.angular_vel or 0)
				local angular_accel = abs(ent.angular_accel or 0)
				local speed_threshold = ent.physics_sleep_velocity_threshold or 0.05
				local accel_threshold = ent.physics_sleep_accel_threshold or 0.01
				local angular_speed_threshold = ent.physics_sleep_angular_velocity_threshold or 0.5
				local angular_accel_threshold = ent.physics_sleep_angular_accel_threshold or 0.05
				if speed_x > speed_threshold or speed_y > speed_threshold or accel_x > accel_threshold or accel_y > accel_threshold or
					angular_speed > angular_speed_threshold or angular_accel > angular_accel_threshold then
					ent:wakePhysics()
				end
			end
			if not ent.ignore_physics then
				if ent.isPhysicsSleeping and ent:isPhysicsSleeping() then
					goto continue
				end
				if not ent.ignore_gravity then
					ent.vel:add(self.layer.gravity, true)
				end
				ent.vel:add(ent.accel, true)
				if not ent.ignore_friction then
					ent.vel:drag(self.layer.friction, true)
				end
				ent.pos:add(ent.vel, true)
				if ent.canRotatePhysics and ent:canRotatePhysics() then
					ent.angular_vel = (ent.angular_vel or 0) + (ent.angular_accel or 0) * _dt
					if not ent.ignore_friction then
						ent.angular_vel *= max(0, 1 - ((ent.angular_damping or 0) * _dt))
					end
					ent.angle = (ent.angle or 0) + ent.angular_vel * _dt
				else
					ent.angular_vel = 0
					ent.angular_accel = 0
				end
				if ent.pos.x ~= prev_x or ent.pos.y ~= prev_y then
					ent._moved_this_frame = true
					moved_count += 1
				end
			end
			::continue::
		end
		return moved_count
	end,

	finalizePhysicsSleep = function(self)
		local physics_entities = self.layer.physics_entities or {}
		local visited = {}
		local islands = {}
		for i = 1, #physics_entities do
			local ent = physics_entities[i]
			if can_island_sleep_entity(ent) and visited[ent] ~= true then
				local island = {}
				local stack = { ent }
				visited[ent] = true
				while #stack > 0 do
					local current = deli(stack, #stack)
					add(island, current)
					local contacts = get_sleep_contacts(current)
					for j = 1, #contacts do
						local other = contacts[j]
						if visited[other] ~= true then
							visited[other] = true
							add(stack, other)
						end
					end
				end
				add(islands, island)
			end
		end

		for i = 1, #islands do
			local island = islands[i]
			local can_sleep_island = true
			local required_frames = 0
			for j = 1, #island do
				local ent = island[j]
				if not island_sleep_candidate(ent) then
					can_sleep_island = false
				end
				required_frames = max(required_frames, get_required_sleep_frames(ent))
			end
			if can_sleep_island then
				local next_idle = 0
				for j = 1, #island do
					next_idle = max(next_idle, (island[j].physics_sleep_idle_frames or 0) + 1)
				end
				for j = 1, #island do
					local ent = island[j]
					ent.physics_sleep_idle_frames = next_idle
					if ent.isPhysicsSleeping and not ent:isPhysicsSleeping() and next_idle >= required_frames then
						ent:sleepPhysics()
					end
				end
			else
				for j = 1, #island do
					local ent = island[j]
					ent.physics_sleep_idle_frames = 0
				end
			end
		end

		local sleeping_count = 0
		for i = 1, #physics_entities do
			local ent = physics_entities[i]
			if ent.isPhysicsSleeping and ent:isPhysicsSleeping() then
				sleeping_count += 1
			end
		end
		return sleeping_count
	end,

	resolveCollisions = function(self)
		if not self.layer.physics_enabled then
			return
		end

		if self.collision_bounds_w ~= self.layer.w or self.collision_bounds_h ~= self.layer.h then
			self.layer.collision:setLayerBounds(0, 0, self.layer.w, self.layer.h)
			self.collision_bounds_w = self.layer.w
			self.collision_bounds_h = self.layer.h
		end
		if self.layer.rebuildCollidableSpatialIndex then
			self.layer:rebuildCollidableSpatialIndex()
		end
		if self.layer.rebuildCollisionBroadphaseSpatialIndex then
			self.layer:rebuildCollisionBroadphaseSpatialIndex()
		end
		if self.layer.collision and self.layer.collision.broadphase then
			self.layer.collision.broadphase.spatial_index = nil
			self.layer.collision.broadphase.dynamic_spatial_index = self.layer.collision_dynamic_spatial_index or self.layer.collidable_spatial_index
			self.layer.collision.broadphase.static_spatial_index = self.layer.collision_static_spatial_index or self.layer.collidable_static_spatial_index
		end
		self.layer.collision:update(self.layer.collidable_entities, self.layer.map_id)
	end,

	sampleProfilerCensus = function(self, profiler)
		if not profiler then
			return
		end
		profiler:observe("layer.sim.entities", #self.layer.entities)
		profiler:observe("layer.sim.physics_entities", #self.layer.physics_entities)
		profiler:observe("layer.sim.collidable_entities", #self.layer.collidable_entities)
		profiler:observe("layer.sim.attached_entities", #self.layer.attached_entities)
		profiler:observe("layer.sim.drawable_entities", #self.layer.drawable_entities)

		local now = time()
		local next_sample_at = self.profiler_census_next_at or 0
		if now < next_sample_at then
			return
		end
		self.profiler_census_next_at = now + 1

		local particle_entities = 0
		local procedural_sprites = 0
		local emitters = 0
		local sprite_nodes = 0
		local attachment_nodes = 0
		for i = 1, #self.layer.entities do
			local ent = self.layer.entities[i]
			local entity_kind = ent and ent.entity_kind or 0
			if entity_kind == ENTITY_KIND_PARTICLE then
				particle_entities += 1
			end
			if entity_kind == ENTITY_KIND_PROCEDURAL or ent.drawProceduralSprite ~= nil then
				procedural_sprites += 1
			end
			if ent.attachment_nodes then
				for j = 1, #ent.attachment_nodes do
					local node = ent.attachment_nodes[j]
					attachment_nodes += 1
					local node_kind = node and node.node_kind or 0
					if node_kind == 2 then
						emitters += 1
					elseif node_kind == 1 then
						sprite_nodes += 1
					end
				end
			end
		end
		profiler:observe("layer.sim.particle_entities", particle_entities)
		profiler:observe("layer.sim.procedural_sprites", procedural_sprites)
		profiler:observe("layer.sim.attachment_nodes", attachment_nodes)
		profiler:observe("layer.sim.sprite_nodes", sprite_nodes)
		profiler:observe("layer.sim.emitters", emitters)
	end,

	update = function(self)
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		self:sampleProfilerCensus(profiler)
		if self.layer.rebuildEntitySpatialIndex then
			self.layer:rebuildEntitySpatialIndex()
		end
		if self.layer.rebuildCollidableSpatialIndex then
			self.layer:rebuildCollidableSpatialIndex()
		end
		self.layer:emitIfListening("layer.pre_update", function()
			return { layer = self.layer }
		end)
		local entities_scope = profiler and profiler:start("layer.sim.update_entities") or nil
		local updated_count, removed_count = self:updateEntities()
		if profiler then profiler:stop(entities_scope) end
		if profiler then
			profiler:observe("layer.sim.updated_entities", updated_count or 0)
			profiler:observe("layer.sim.removed_entities", removed_count or 0)
		end
		if (updated_count or 0) > 0 or (removed_count or 0) > 0 then
			self.layer.entity_spatial_dirty = true
			self.layer.collidable_spatial_dirty = true
		end
		self.layer:emitIfListening("layer.post_objects", function()
			return { layer = self.layer }
		end)
		local physics_scope = profiler and profiler:start("layer.sim.apply_physics") or nil
		local moved_count = self:applyPhysics()
		if profiler then profiler:stop(physics_scope) end
		if profiler then profiler:observe("layer.sim.moved_entities", moved_count or 0) end
		if (moved_count or 0) > 0 then
			self.layer.entity_spatial_dirty = true
			self.layer.collidable_spatial_dirty = true
		end
		self.layer:emitIfListening("layer.post_physics", function()
			return { layer = self.layer }
		end)
		local collision_scope = profiler and profiler:start("layer.sim.resolve_collisions") or nil
		self:resolveCollisions()
		if profiler then profiler:stop(collision_scope) end
		local sleeping_count = self:finalizePhysicsSleep()
		if profiler then profiler:observe("layer.sim.sleeping_physics_entities", sleeping_count or 0) end
		self.layer:emitIfListening("layer.post_collision", function()
			return { layer = self.layer }
		end)
		local camera_scope = profiler and profiler:start("layer.sim.camera_update") or nil
		self.layer.camera:update()
		if profiler then profiler:stop(camera_scope) end
		local attachment_scope = profiler and profiler:start("layer.sim.sync_attachments") or nil
		local synced_count = self:syncAttachments()
		if profiler then profiler:stop(attachment_scope) end
		if profiler then profiler:observe("layer.sim.synced_attachments", synced_count or 0) end
		self.layer:emitIfListening("layer.post_update", function()
			return { layer = self.layer }
		end)
	end,
})

return Simulation
