local Vector = include("src/Engine/Math/Vector.lua")

local TransformAttachments = {}

TransformAttachments.emit = function(self, name, payload)
	if self.owner and self.owner.emitEvent then
		self.owner:emitEvent(name, payload)
	end
end

TransformAttachments.createAttachment = function(self, parent_transform, slot_name, config)
	config = config or {}
	local offset = config.offset or config.slot_offset or self.local_position or config
	local local_position = Vector:new({
		x = offset and offset.x or 0,
		y = offset and offset.y or 0,
	})
	return {
		parent_transform = parent_transform,
		parent = parent_transform.owner,
		slot_name = slot_name or "origin",
		offset = local_position,
		enabled = config.enabled ~= false,
		sync_position = config.sync_position ~= false,
		sync_when_disabled = config.sync_when_disabled ~= false,
		forward_collisions = config.forward_collisions ~= false,
	}
end

TransformAttachments.getAttachment = function(self)
	return self.attachment
end

TransformAttachments.isAttached = function(self)
	return self.attachment ~= nil and self.attachment.parent_transform ~= nil
end

TransformAttachments.isAttachmentEnabled = function(self)
	return self.attachment and self.attachment.enabled ~= false or false
end

TransformAttachments.setAttachmentEnabled = function(self, enabled)
	if not self.attachment then
		return false
	end
	self.attachment.enabled = enabled ~= false
	return true
end

TransformAttachments.shouldSync = function(self)
	if not self.attachment or not self.attachment.parent_transform then
		return false
	end
	if self.attachment.sync_position == false then
		return false
	end
	local slot = self.attachment.parent_transform:getSlot(self.attachment.slot_name)
	if not slot then
		return false
	end
	if self.attachment.enabled == false and self.attachment.sync_when_disabled == false then
		return false
	end
	if slot.enabled == false and slot.sync_when_disabled == false then
		return false
	end
	return true
end

TransformAttachments.attachTo = function(self, parent_transform, slot_name, config)
	if not parent_transform or not parent_transform.owner then
		return false
	end
	local parent_owner = parent_transform.owner
	local slot = parent_transform:getSlot(slot_name)
	if not parent_transform:canAttachToSlot(slot, self.owner) then
		return false
	end
	if self:isAttached() then
		self:detach()
	end

	self.attachment = self:createAttachment(parent_transform, slot_name, config)
	self.local_position = self.attachment.offset
	add(slot.attachments, self)

	if self.owner then
		self.owner.parent = parent_owner
		self.owner.parent_slot = self.attachment.slot_name
		if parent_owner.layer then
			self.owner.layer = parent_owner.layer
		end
		if self.owner.is_world_object and self.owner.layer and self.owner.layer.refreshAttachmentBucket then
			self.owner.layer:refreshAttachmentBucket(self.owner)
		end
	end

	if parent_owner.on_child_attached then
		parent_owner:on_child_attached(self.owner, slot.name, self.attachment)
	end
	if self.owner and self.owner.on_attached then
		self.owner:on_attached(parent_owner, slot.name, self.attachment)
	end
	self:emit("attachment.attached", {
		parent = parent_owner,
		slot_name = slot.name,
		attachment = self.attachment,
	})
	self:sync()
	return true
end

TransformAttachments.detach = function(self)
	if not self.attachment then
		return false
	end
	local attachment = self.attachment
	local parent_transform = attachment.parent_transform
	local parent_owner = attachment.parent
	local slot_name = attachment.slot_name
	if parent_transform then
		parent_transform:removeChildFromSlot(self, slot_name)
		if parent_owner and parent_owner.on_child_detached then
			parent_owner:on_child_detached(self.owner, slot_name, attachment)
		end
	end
	if self.owner and self.owner.on_detached then
		self.owner:on_detached(parent_owner, slot_name, attachment)
	end
	self:emit("attachment.detached", {
		parent = parent_owner,
		slot_name = slot_name,
		attachment = attachment,
	})
	self.attachment = nil
	if self.owner then
		self.owner.parent = nil
		self.owner.parent_slot = nil
		if self.owner.is_world_object and self.owner.layer and self.owner.layer.refreshAttachmentBucket then
			self.owner.layer:refreshAttachmentBucket(self.owner)
		end
	end
	self.local_position.x = self.position.x
	self.local_position.y = self.position.y
	return true
end

TransformAttachments.sync = function(self)
	if not self:shouldSync() then
		return
	end
	local attachment = self.attachment
	local anchor = attachment.parent_transform:getSlotWorldPosition(attachment.slot_name)
	self.position.x = anchor.x + self.local_position.x
	self.position.y = anchor.y + self.local_position.y
end

return TransformAttachments
