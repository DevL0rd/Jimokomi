local Vector = include("src/Engine/Math/Vector.lua")

local TransformSlots = {}

TransformSlots.ensureSlots = function(self)
	if self.slots.origin == nil then
		self.slots.origin = {
			name = "origin",
			pos = Vector:new({ x = 0, y = 0 }),
			enabled = true,
			sync_when_disabled = true,
			destroy_policy = self.default_slot_destroy_policy,
			max_children = nil,
			attachments = {},
			accepts = nil,
		}
	end
end

TransformSlots.createSlotRecord = function(self, name, config)
	config = config or {}
	local pos = config.pos or config
	return {
		name = name,
		pos = Vector:new({
			x = pos and pos.x or 0,
			y = pos and pos.y or 0,
		}),
		enabled = config.enabled ~= false,
		sync_when_disabled = config.sync_when_disabled ~= false,
		destroy_policy = config.destroy_policy or self.default_slot_destroy_policy,
		max_children = config.max_children,
		attachments = {},
		accepts = config.accepts,
	}
end

TransformSlots.applySlotDefinitions = function(self)
	if not self.slot_defs then
		return
	end
	for name, config in pairs(self.slot_defs) do
		self:defineSlot(name, config)
	end
end

TransformSlots.defineSlot = function(self, name, config)
	self:ensureSlots()
	self.slots[name] = self:createSlotRecord(name, config)
	return self.slots[name]
end

TransformSlots.getSlot = function(self, name)
	self:ensureSlots()
	return self.slots[name or "origin"] or self.slots.origin
end

TransformSlots.getSlotNames = function(self)
	self:ensureSlots()
	local names = {}
	for name in pairs(self.slots) do
		add(names, name)
	end
	return names
end

TransformSlots.hasSlot = function(self, name)
	self:ensureSlots()
	if name == nil then
		return self.slots.origin ~= nil
	end
	return self.slots[name] ~= nil
end

TransformSlots.setSlotPosition = function(self, name, pos)
	local slot = self:getSlot(name)
	if not slot then
		return nil
	end
	slot.pos = Vector:new({
		x = pos and pos.x or 0,
		y = pos and pos.y or 0,
	})
	return slot.pos
end

TransformSlots.getSlotLocalPosition = function(self, name)
	local slot = self:getSlot(name)
	return slot and slot.pos or Vector:new()
end

TransformSlots.getSlotWorldPosition = function(self, name)
	local slot_pos = self:getSlotLocalPosition(name)
	return Vector:new({
		x = self.position.x + slot_pos.x,
		y = self.position.y + slot_pos.y,
	})
end

TransformSlots.isSlotEnabled = function(self, name)
	local slot = self:getSlot(name)
	return slot and slot.enabled ~= false or false
end

TransformSlots.setSlotEnabled = function(self, name, enabled)
	local slot = self:getSlot(name)
	if not slot then
		return false
	end
	local was_enabled = slot.enabled ~= false
	slot.enabled = enabled ~= false
	if was_enabled ~= slot.enabled and self.owner then
		if slot.enabled and self.owner.on_slot_enabled then
			self.owner:on_slot_enabled(name, slot)
		elseif (not slot.enabled) and self.owner.on_slot_disabled then
			self.owner:on_slot_disabled(name, slot)
		end
		self:emit(slot.enabled and "slot.enabled" or "slot.disabled", {
			slot_name = name,
			slot = slot,
		})
	end
	return true
end

TransformSlots.setSlotsEnabled = function(self, enabled)
	self.slots_enabled = enabled ~= false
	return self.slots_enabled
end

TransformSlots.canAttachToSlot = function(self, slot, child_owner)
	if self.slots_enabled == false or not slot then
		return false
	end
	if slot.max_children and #slot.attachments >= slot.max_children then
		return false
	end
	if slot.accepts == nil or child_owner == nil then
		return true
	end
	for _, accepted in pairs(slot.accepts) do
		if child_owner._type == accepted or sub(child_owner._type or "", 1, #accepted) == accepted then
			return true
		end
	end
	return false
end

TransformSlots.removeChildFromSlot = function(self, child_transform, slot_name)
	local slot = self:getSlot(slot_name)
	if not slot then
		return
	end
	del(slot.attachments, child_transform)
end

TransformSlots.getAttachedChildren = function(self, slot_name)
	local attached = {}
	if slot_name then
		local slot = self:getSlot(slot_name)
		if not slot then
			return attached
		end
		for _, child_transform in pairs(slot.attachments) do
			if child_transform.owner then
				add(attached, child_transform.owner)
			end
		end
		return attached
	end

	for _, slot in pairs(self.slots) do
		for _, child_transform in pairs(slot.attachments) do
			if child_transform.owner then
				add(attached, child_transform.owner)
			end
		end
	end
	return attached
end

TransformSlots.destroySlot = function(self, name, destroy_policy)
	if name == nil or name == "origin" then
		return false
	end
	local slot = self:getSlot(name)
	if not slot or self.slots[name] == nil then
		return false
	end

	local attachments = {}
	for _, child_transform in pairs(slot.attachments) do
		add(attachments, child_transform)
	end

	local policy = destroy_policy or slot.destroy_policy or self.default_slot_destroy_policy
	for _, child_transform in pairs(attachments) do
		local child_owner = child_transform.owner
		if policy == "destroy_children" then
			child_owner:destroy()
		else
			child_transform:detach()
		end
	end

	self.slots[name] = nil
	if self.owner and self.owner.on_slot_destroyed then
		self.owner:on_slot_destroyed(name, slot)
	end
	self:emit("slot.destroyed", {
		slot_name = name,
		slot = slot,
	})
	return true
end

TransformSlots.destroySlots = function(self)
	local names = self:getSlotNames()
	for _, name in pairs(names) do
		if name ~= "origin" then
			self:destroySlot(name)
		end
	end
end

return TransformSlots
