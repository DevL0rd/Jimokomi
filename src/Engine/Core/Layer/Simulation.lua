local Class = include("src/Engine/Core/Class.lua")

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
		for i = #self.layer.entities, 1, -1 do
			local ent = self.layer.entities[i]
			if not ent._delete then
				if ent.needsUpdate == nil or ent:needsUpdate() then
					ent:update()
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
			if not ent.ignore_physics then
				if not ent.ignore_gravity then
					ent.vel:add(self.layer.gravity, true)
					ent.vel:add(ent.accel, true)
				end
				if not ent.ignore_friction then
					ent.vel:drag(self.layer.friction, true)
				end
				ent.pos:add(ent.vel, true)
				moved_count += 1
			end
		end
		return moved_count
	end,

	resolveCollisions = function(self)
		if not self.layer.physics_enabled then
			return
		end

		self.layer.collision:setLayerBounds(0, 0, self.layer.w, self.layer.h)
		self.layer.collision:update(self.layer.collidable_entities, self.layer.map_id)
	end,

	update = function(self)
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		if profiler then
			profiler:observe("layer.sim.entities", #self.layer.entities)
			profiler:observe("layer.sim.physics_entities", #self.layer.physics_entities)
			profiler:observe("layer.sim.collidable_entities", #self.layer.collidable_entities)
			profiler:observe("layer.sim.attached_entities", #self.layer.attached_entities)
			profiler:observe("layer.sim.drawable_entities", #self.layer.drawable_entities)
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
		self.layer:emitIfListening("layer.post_objects", function()
			return { layer = self.layer }
		end)
		local physics_scope = profiler and profiler:start("layer.sim.apply_physics") or nil
		local moved_count = self:applyPhysics()
		if profiler then profiler:stop(physics_scope) end
		if profiler then profiler:observe("layer.sim.moved_entities", moved_count or 0) end
		self.layer:emitIfListening("layer.post_physics", function()
			return { layer = self.layer }
		end)
		local collision_scope = profiler and profiler:start("layer.sim.resolve_collisions") or nil
		self:resolveCollisions()
		if profiler then profiler:stop(collision_scope) end
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
