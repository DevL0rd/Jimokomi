local Class = include("src/Engine/Core/Class.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local Transform = include("src/Engine/World/Transform.lua")

local AttachmentNode = Class:new({
	_type = "AttachmentNode",
	is_world_object = false,
	parent = nil,
	parent_slot = nil,
	pos = nil,
	slot_offset = nil,
	slot_defs = nil,
	slots_enabled = true,
	default_slot_destroy_policy = "destroy_children",
	layer = nil,
	children = nil,

	init = function(self)
		local initial_pos = self.pos or Vector:new()
		self.transform = Transform:new({
			owner = self,
			position = initial_pos,
			local_position = self.slot_offset,
			slot_defs = self.slot_defs,
			slots_enabled = self.slots_enabled ~= false,
			default_slot_destroy_policy = self.default_slot_destroy_policy,
		})
		self.pos = self.transform.position
		self.slot_offset = self.transform:getSlotOffset()
		self.children = self.children or {}
		if self.parent then
			self:attachTo(self.parent, self.parent_slot, {
				offset = self.slot_offset,
			})
		end
	end,

	getTransform = function(self)
		return self.transform
	end,

	shouldSyncToParent = function(self)
		return self.transform and self.transform:shouldSync() or false
	end,

	syncToParent = function(self)
		if self.transform then
			self.transform:sync()
		end
	end,

	getWorld = function(self)
		return self.parent and self.parent.getWorld and self.parent:getWorld() or nil
	end,

	addAttachmentNode = function(self, child)
		add(self.children, child)
		child.layer = self.layer
	end,

	removeAttachmentNode = function(self, child)
		del(self.children, child)
	end,

	attachTo = function(self, parent, slot_name, config)
		if not parent or not parent.getTransform then
			return false
		end
		if self.parent and self.parent.removeAttachmentNode then
			self.parent:removeAttachmentNode(self)
		end
		self.parent = parent
		self.parent_slot = slot_name
		self.layer = parent.layer
		if parent.addAttachmentNode then
			parent:addAttachmentNode(self)
		end
		return self.transform:attachTo(parent:getTransform(), slot_name, config)
	end,

	detach = function(self)
		if self.parent and self.parent.removeAttachmentNode then
			self.parent:removeAttachmentNode(self)
		end
		self.parent = nil
		self.parent_slot = nil
		self.layer = nil
		return self.transform:detach()
	end,

	defineSlot = function(self, name, config)
		return self.transform:defineSlot(name, config)
	end,

	hasSlot = function(self, name)
		return self.transform:hasSlot(name)
	end,

	getSlot = function(self, name)
		return self.transform:getSlot(name)
	end,

	getSlotWorldPosition = function(self, name)
		return self.transform:getSlotWorldPosition(name)
	end,

	setLocalPosition = function(self, pos)
		return self.transform:setLocalPosition(pos)
	end,

	getLocalPosition = function(self)
		return self.transform:getLocalPosition()
	end,

	updateNode = function(self)
	end,

	drawNode = function(self)
	end,

	updateRecursive = function(self)
		if self.transform and self.transform:shouldSync() then
			self.transform:sync()
		end
		self:updateNode()
		for i = 1, #self.children do
			self.children[i]:updateRecursive()
		end
	end,

	drawRecursive = function(self)
		self:drawNode()
		for i = 1, #self.children do
			self.children[i]:drawRecursive()
		end
	end,

	destroy = function(self)
		for i = #self.children, 1, -1 do
			self.children[i]:destroy()
		end
		self.children = {}
		self.transform:destroySlots()
		if self.transform:isAttached() then
			self.transform:detach()
		end
		if self.parent and self.parent.removeAttachmentNode then
			self.parent:removeAttachmentNode(self)
		end
		self.parent = nil
		self.parent_slot = nil
		self.layer = nil
	end,
})

return AttachmentNode
