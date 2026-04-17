local TransformSnapshot = {}

TransformSnapshot.toSnapshot = function(self)
	local slots = {}
	for name, slot in pairs(self.slots) do
		slots[name] = {
			pos = { x = slot.pos.x, y = slot.pos.y },
			enabled = slot.enabled ~= false,
			sync_when_disabled = slot.sync_when_disabled ~= false,
			destroy_policy = slot.destroy_policy,
			max_children = slot.max_children,
			accepts = slot.accepts,
		}
	end

	local attachment = nil
	if self.attachment and self.attachment.parent then
		attachment = {
			parent_id = self.attachment.parent.object_id,
			slot_name = self.attachment.slot_name,
			offset = {
				x = self.local_position.x,
				y = self.local_position.y,
			},
			enabled = self.attachment.enabled ~= false,
			sync_position = self.attachment.sync_position ~= false,
			sync_when_disabled = self.attachment.sync_when_disabled ~= false,
			forward_collisions = self.attachment.forward_collisions ~= false,
		}
	end

	return {
		position = { x = self.position.x, y = self.position.y },
		local_position = { x = self.local_position.x, y = self.local_position.y },
		slots_enabled = self.slots_enabled ~= false,
		slots = slots,
		attachment = attachment,
	}
end

TransformSnapshot.applySnapshot = function(self, snapshot)
	if not snapshot then
		return
	end
	self:setWorldPosition(snapshot.position or {})
	self:setLocalPosition(snapshot.local_position or snapshot.position or {})
	self.slots_enabled = snapshot.slots_enabled ~= false
	self.slots = {}
	self:ensureSlots()
	for name, slot in pairs(snapshot.slots or {}) do
		self:defineSlot(name, slot)
	end
	self.pending_attachment_snapshot = snapshot.attachment
end

return TransformSnapshot
