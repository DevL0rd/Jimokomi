local WorldObjectSnapshot = {}

WorldObjectSnapshot.getDebugSnapshot = function(self)
	local attachment = self:getAttachment()
	return {
		object_id = self.object_id,
		type = self._type,
		pos = { x = self.pos.x, y = self.pos.y },
		shape = self.collider and self.collider:toSnapshot() or nil,
		attachment = attachment and {
			parent = attachment.parent and attachment.parent._type or nil,
			slot_name = attachment.slot_name,
		} or nil,
		slot_count = #(self.transform and self.transform:getSlotNames() or {}),
	}
end

WorldObjectSnapshot.toSnapshot = function(self)
	return {
		_type = self._type,
		object_id = self.object_id,
		name = self.name,
		transform = self.transform and self.transform:toSnapshot() or nil,
		collider = self.collider and self.collider:toSnapshot() or nil,
		vel = { x = self.vel.x, y = self.vel.y },
		accel = { x = self.accel.x, y = self.accel.y },
		friction = self.friction,
		lifetime = self.lifetime,
		flags = {
			debug = self.debug,
			ignore_physics = self.ignore_physics,
			ignore_gravity = self.ignore_gravity,
			ignore_friction = self.ignore_friction,
			ignore_collisions = self.ignore_collisions,
		},
		custom_state = self.exportState and self:exportState() or nil,
	}
end

WorldObjectSnapshot.applySnapshot = function(self, snapshot, clone_vector)
	if not snapshot then
		return
	end
	self.object_id = snapshot.object_id or self.object_id
	if self.transform then
		self.transform:applySnapshot(snapshot.transform or {})
		self.pos = self.transform.position
		self.slot_offset = self.transform:getSlotOffset()
	end
	if self.collider then
		self.collider:applySnapshot(snapshot.collider or {})
	end
	self.vel = clone_vector(snapshot.vel)
	self.accel = clone_vector(snapshot.accel)
	self.friction = snapshot.friction or self.friction
	self.lifetime = snapshot.lifetime or self.lifetime
	local flags = snapshot.flags or {}
	self.debug = flags.debug == true
	self.ignore_physics = flags.ignore_physics == true
	self.ignore_gravity = flags.ignore_gravity == true
	self.ignore_friction = flags.ignore_friction == true
	self.ignore_collisions = flags.ignore_collisions == true
	if self.collider then
		self.collider:setEnabled(not self.ignore_collisions)
	end
	if snapshot.custom_state and self.importState then
		self:importState(snapshot.custom_state)
	end
end

return WorldObjectSnapshot
