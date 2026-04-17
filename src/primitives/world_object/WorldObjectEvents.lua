local WorldObjectEvents = {}

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

WorldObjectEvents.didAddToLayer = function(self, layer)
	self.layer = layer
	if self.object_id == nil and layer.engine and layer.engine.nextObjectId then
		self.object_id = layer.engine:nextObjectId()
	end
	for _, child in pairs(self:getAttachedChildren()) do
		child.layer = layer
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
	if self.debug then
		local collision_scale = 10
		local end_x = self.pos.x + (vector.x * collision_scale)
		local end_y = self.pos.y + (vector.y * collision_scale)
		self.layer.gfx:line(self.pos.x, self.pos.y, end_x, end_y, 8)
		self.layer.gfx:circfill(end_x, end_y, 2, 8)
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

WorldObjectEvents.on_overlap = function(self, ent)
	self:emitEvent("object.overlap", {
		other = ent,
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
