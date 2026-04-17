local Container = include("src/classes/Container.lua")

local Equipment = Container:new({
	_type = "Equipment",
	slot_names = nil,

	init = function(self)
		self.slots = {}
		self.slot_keys = {}
		for _, slot_name in pairs(self.slot_names or {}) do
			add(self.slot_keys, slot_name)
		end
	end,

	canAcceptItem = function(self, item, slot_key)
		if not item then
			return false
		end
		if item.equipment_slot and slot_key then
			return item.equipment_slot == slot_key
		end
		return true
	end,

	equip = function(self, item, slot_key)
		if not self:canAcceptItem(item, slot_key) then
			return false
		end
		self.slots[slot_key] = item:toItemStack(1)
		return true
	end,

	unequip = function(self, slot_key)
		local stack = self.slots[slot_key]
		self.slots[slot_key] = nil
		return stack
	end,
})

return Equipment
