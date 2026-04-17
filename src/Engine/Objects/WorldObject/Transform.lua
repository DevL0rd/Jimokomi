local WorldObjectTransform = {}

WorldObjectTransform.getTransform = function(self)
	return self.transform
end

WorldObjectTransform.setWorldPosition = function(self, pos)
	return self.transform:setWorldPosition(pos)
end

WorldObjectTransform.setLocalPosition = function(self, pos)
	return self.transform:setLocalPosition(pos)
end

WorldObjectTransform.getLocalPosition = function(self)
	return self.transform:getLocalPosition()
end

WorldObjectTransform.translate = function(self, vec, scale_with_dt)
	return self.transform:translate(vec, scale_with_dt)
end

WorldObjectTransform.ensureSlots = function(self)
	return self.transform:ensureSlots()
end

WorldObjectTransform.defineSlot = function(self, name, config)
	return self.transform:defineSlot(name, config)
end

WorldObjectTransform.getSlot = function(self, name)
	return self.transform:getSlot(name)
end

WorldObjectTransform.hasSlot = function(self, name)
	return self.transform:hasSlot(name)
end

WorldObjectTransform.setSlotPosition = function(self, name, pos)
	return self.transform:setSlotPosition(name, pos)
end

WorldObjectTransform.getSlotLocalPosition = function(self, name)
	return self.transform:getSlotLocalPosition(name)
end

WorldObjectTransform.getSlotWorldPosition = function(self, name)
	return self.transform:getSlotWorldPosition(name)
end

WorldObjectTransform.isSlotEnabled = function(self, name)
	return self.transform:isSlotEnabled(name)
end

WorldObjectTransform.setSlotEnabled = function(self, name, enabled)
	return self.transform:setSlotEnabled(name, enabled)
end

WorldObjectTransform.setSlotsEnabled = function(self, enabled)
	return self.transform:setSlotsEnabled(enabled)
end

WorldObjectTransform.setSlotOffset = function(self, pos)
	return self.transform:setLocalPosition(pos)
end

WorldObjectTransform.getSlotOffset = function(self)
	return self.transform:getSlotOffset()
end

WorldObjectTransform.getAttachment = function(self)
	return self.transform:getAttachment()
end

WorldObjectTransform.isAttached = function(self)
	return self.transform:isAttached()
end

WorldObjectTransform.isAttachmentEnabled = function(self)
	return self.transform:isAttachmentEnabled()
end

WorldObjectTransform.setAttachmentEnabled = function(self, enabled)
	return self.transform:setAttachmentEnabled(enabled)
end

WorldObjectTransform.shouldSyncToParent = function(self)
	return self.transform:shouldSync()
end

WorldObjectTransform.attachTo = function(self, parent, slot_name, config)
	if not parent or not parent.transform then
		return false
	end
	return self.transform:attachTo(parent.transform, slot_name, config)
end

WorldObjectTransform.attachChild = function(self, child, slot_name, config)
	child:attachTo(self, slot_name, config)
	return child
end

WorldObjectTransform.getAttachedChildren = function(self, slot_name)
	return self.transform:getAttachedChildren(slot_name)
end

WorldObjectTransform.detach = function(self)
	return self.transform:detach()
end

WorldObjectTransform.syncToParent = function(self)
	return self.transform:sync()
end

WorldObjectTransform.destroySlot = function(self, name, destroy_policy)
	return self.transform:destroySlot(name, destroy_policy)
end

WorldObjectTransform.destroySlots = function(self)
	return self.transform:destroySlots()
end

return WorldObjectTransform
