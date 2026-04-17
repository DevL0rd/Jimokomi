local Class = include("src/classes/Class.lua")

local LayerSimulation = Class:new({
	_type = "LayerSimulation",
	layer = nil,

	syncAttachments = function(self)
		for i = 1, #self.layer.attached_entities do
			local ent = self.layer.attached_entities[i]
			if ent.parent and ent.getTransform and ent:getTransform() and ent:shouldSyncToParent() then
				ent:syncToParent()
			end
		end
	end,

	updateEntities = function(self)
		for i = #self.layer.entities, 1, -1 do
			local ent = self.layer.entities[i]
			if not ent._delete then
				if ent.needsUpdate == nil or ent:needsUpdate() then
					ent:update()
				end
			else
				ent:willRemoveFromLayer(self.layer)
				ent:unInit()
				ent:didRemoveFromLayer(self.layer)
				self.layer:unregisterEntityBuckets(ent)
				del(self.layer.entities, ent)
			end
		end
	end,

	applyPhysics = function(self)
		if not self.layer.physics_enabled then
			return
		end

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
			end
		end
	end,

	resolveCollisions = function(self)
		if not self.layer.physics_enabled then
			return
		end

		self.layer.collision:setLayerBounds(0, 0, self.layer.w, self.layer.h)
		self.layer.collision:update(self.layer.collidable_entities, self.layer.map_id)
	end,

	update = function(self)
		self.layer:emit("layer.pre_update", { layer = self.layer })
		self:updateEntities()
		self.layer:emit("layer.post_objects", { layer = self.layer })
		self:applyPhysics()
		self.layer:emit("layer.post_physics", { layer = self.layer })
		self:resolveCollisions()
		self.layer:emit("layer.post_collision", { layer = self.layer })
		self.layer.camera:update()
		self:syncAttachments()
		self.layer:emit("layer.post_update", { layer = self.layer })
	end,
})

return LayerSimulation
