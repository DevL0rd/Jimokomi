local WorldObjectEvents = {}

local function has_event_listener(self, name)
	local layer = self and self.layer or nil
	if layer and layer.events and layer.events.hasListeners then
		return layer.events:hasListeners(name, true)
	end
	if layer and layer.engine and layer.engine.events and layer.engine.events.hasListeners then
		return layer.engine.events:hasListeners(name, true)
	end
	return false
end

WorldObjectEvents.emitEvent = function(self, name, payload)
	payload = payload or {}
	if payload.object == nil then
		payload.object = self
	end
	if self.layer and self.layer.emit then
		self.layer:emit(name, payload)
	elseif self.layer and self.layer.engine and self.layer.engine.emit then
		self.layer.engine:emit(name, payload)
	end
end

WorldObjectEvents.hasCollisionEnterInterest = function(self)
	return self.debug_collision_interest == true or
		self.on_collision ~= WorldObjectEvents.on_collision or
		self.on_collision_enter ~= WorldObjectEvents.on_collision_enter or
		has_event_listener(self, "object.collision")
end

WorldObjectEvents.hasCollisionStayInterest = function(self)
	return self.on_collision_stay ~= nil and self.on_collision_stay ~= WorldObjectEvents.on_collision_stay
end

WorldObjectEvents.hasCollisionExitInterest = function(self)
	return self.on_collision_exit ~= WorldObjectEvents.on_collision_exit or
		has_event_listener(self, "object.collision_exit")
end

WorldObjectEvents.hasAnyCollisionInterest = function(self)
	return self:hasCollisionEnterInterest() or
		self:hasCollisionStayInterest() or
		self:hasCollisionExitInterest()
end

WorldObjectEvents.hasOverlapEnterInterest = function(self)
	return self.on_overlap ~= WorldObjectEvents.on_overlap or
		self.on_overlap_enter ~= WorldObjectEvents.on_overlap_enter or
		has_event_listener(self, "object.overlap")
end

WorldObjectEvents.hasOverlapStayInterest = function(self)
	return self.on_overlap_stay ~= nil and self.on_overlap_stay ~= WorldObjectEvents.on_overlap_stay
end

WorldObjectEvents.hasOverlapExitInterest = function(self)
	return self.on_overlap_exit ~= WorldObjectEvents.on_overlap_exit or
		has_event_listener(self, "object.overlap_exit")
end

WorldObjectEvents.hasAnyOverlapInterest = function(self)
	return self:hasOverlapEnterInterest() or
		self:hasOverlapStayInterest() or
		self:hasOverlapExitInterest()
end

WorldObjectEvents.didAddToLayer = function(self, layer)
	self.layer = layer
	if self.object_id == nil and layer.engine and layer.engine.nextObjectId then
		self.object_id = layer.engine:nextObjectId()
	end
	for _, child in pairs(self:getAttachedChildren()) do
		child.layer = layer
	end
	if self.attachment_nodes then
		for i = 1, #self.attachment_nodes do
			self.attachment_nodes[i].layer = layer
		end
	end
	if self.on_added_to_layer then
		self:on_added_to_layer(layer)
	end
	self:emitEvent("object.added_to_layer", {
		layer = layer,
	})
end

WorldObjectEvents.willRemoveFromLayer = function(self, layer)
	if self.on_removed_from_layer then
		self:on_removed_from_layer(layer)
	end
end

WorldObjectEvents.didRemoveFromLayer = function(self, layer)
	self:emitEvent("object.removed_from_layer", {
		layer = layer,
	})
	if self.layer == layer then
		self.layer = nil
	end
end

WorldObjectEvents.on_collision = function(self, ent, vector)
	if self.debug_collision_interest == true then
		local collision_scale = 10
		local end_x = self.pos.x + (vector.x * collision_scale)
		local end_y = self.pos.y + (vector.y * collision_scale)
		local sx, sy = self.layer.camera:layerToScreenXY(self.pos.x, self.pos.y)
		local ex, ey = self.layer.camera:layerToScreenXY(end_x, end_y)
		self.layer.gfx:screenLine(sx, sy, ex, ey, 8)
		self.layer.gfx:screenCircfill(ex, ey, 2, 8)
	end
	self:emitEvent("object.collision", {
		other = ent,
		vector = vector,
	})
	local attachment = self:getAttachment()
	if self.parent and self:isAttachmentEnabled() and attachment and attachment.forward_collisions ~= false then
		self.parent:on_collision(ent, vector)
	end
end

WorldObjectEvents.on_collision_enter = function(self, ent, vector)
	self:on_collision(ent, vector)
end

WorldObjectEvents.on_collision_stay = function(self, ent, vector)
end

WorldObjectEvents.on_collision_exit = function(self, ent, vector)
	self:emitEvent("object.collision_exit", {
		other = ent,
		vector = vector,
	})
end

WorldObjectEvents.on_overlap = function(self, ent)
	self:emitEvent("object.overlap", {
		other = ent,
	})
end

WorldObjectEvents.on_overlap_enter = function(self, ent, vector)
	self:on_overlap(ent, vector)
end

WorldObjectEvents.on_overlap_stay = function(self, ent, vector)
end

WorldObjectEvents.on_overlap_exit = function(self, ent, vector)
	self:emitEvent("object.overlap_exit", {
		other = ent,
		vector = vector,
	})
end

WorldObjectEvents.destroy = function(self)
	if self:isAttached() then
		self:detach()
	end
	self:destroySlots()
	self:emitEvent("object.destroyed", {})
	if self.on_destroy then
		self:on_destroy()
	end
	self._delete = true
end

return WorldObjectEvents
