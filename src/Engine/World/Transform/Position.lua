local Vector = include("src/Engine/Math/Vector.lua")

local TransformPosition = {}

TransformPosition.init = function(self)
	local start_pos = self.position or Vector:new()
	self.position = Vector:new({
		x = start_pos.x or 0,
		y = start_pos.y or 0,
	})
	self.local_position = Vector:new({
		x = self.local_position and self.local_position.x or self.position.x,
		y = self.local_position and self.local_position.y or self.position.y,
	})
end

TransformPosition.setLocalPosition = function(self, pos)
	self.local_position.x = pos and pos.x or 0
	self.local_position.y = pos and pos.y or 0
	if self:isAttached() then
		if self.attachment then
			self.attachment.offset = self.local_position
		end
		self:sync()
	else
		self.position.x = self.local_position.x
		self.position.y = self.local_position.y
	end
	return self.local_position
end

TransformPosition.getLocalPosition = function(self)
	return self.local_position
end

TransformPosition.setWorldPosition = function(self, pos)
	self.position.x = pos and pos.x or 0
	self.position.y = pos and pos.y or 0

	if self:isAttached() then
		local anchor = self.attachment.parent_transform:getSlotWorldPosition(self.attachment.slot_name)
		self.local_position.x = self.position.x - anchor.x
		self.local_position.y = self.position.y - anchor.y
		self.attachment.offset = self.local_position
	else
		self.local_position.x = self.position.x
		self.local_position.y = self.position.y
	end

	return self.position
end

TransformPosition.translate = function(self, vec, scale_with_dt)
	if scale_with_dt then
		self:setWorldPosition({
			x = self.position.x + vec.x * _dt,
			y = self.position.y + vec.y * _dt,
		})
		return
	end
	self:setWorldPosition({
		x = self.position.x + vec.x,
		y = self.position.y + vec.y,
	})
end

TransformPosition.getSlotOffset = function(self)
	return self.local_position
end

return TransformPosition
