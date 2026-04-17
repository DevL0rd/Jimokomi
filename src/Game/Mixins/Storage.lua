local Inventory = include("src/Game/Mixins/Storage/Inventory.lua")
local Equipment = include("src/Game/Mixins/Storage/Equipment.lua")

local Storage = {}

Storage.mixin = function(self)
	if self._storage_mixin_applied then
		return
	end

	self._storage_mixin_applied = true

	for key, value in pairs(Storage) do
		if key ~= "mixin" and key ~= "init" and type(value) == "function" and self[key] == nil then
			self[key] = value
		end
	end
end

Storage.init = function(self)
	Storage.mixin(self)

	if self.inventory_slot_count and not self.inventory then
		self.inventory = Inventory:new({
			owner = self,
			slot_count = self.inventory_slot_count,
			slot_max_stack_size = self.inventory_slot_max_stack_size,
			slot_limits = self.inventory_slot_limits,
			accepts_tags = self.inventory_accepts_tags,
		})
	end

	if self.equipment_slots and not self.equipment then
		self.equipment = Equipment:new({
			owner = self,
			slot_names = self.equipment_slots,
			slot_max_stack_size = self.equipment_slot_max_stack_size,
			slot_limits = self.equipment_slot_limits,
		})
	end
end

Storage.getPrimaryInventory = function(self)
	return self.inventory
end

Storage.getStorageState = function(self)
	return {
		inventory = self.inventory and self.inventory:toSnapshot() or nil,
		equipment = self.equipment and self.equipment:toSnapshot() or nil,
	}
end

Storage.applyStorageState = function(self, state)
	if not state then
		return
	end
	if state.inventory and self.inventory then
		self.inventory:applySnapshot(state.inventory)
	end
	if state.equipment and self.equipment then
		self.equipment:applySnapshot(state.equipment)
	end
end

Storage.transferInventorySlotTo = function(self, target_owner, from_slot_key, amount, to_slot_key)
	local source = self.inventory
	local target = target_owner and target_owner.getPrimaryInventory and target_owner:getPrimaryInventory() or target_owner and target_owner.inventory
	if not source or not target then
		return 0
	end
	return source:transferTo(target, from_slot_key, amount, to_slot_key)
end

Storage.useInventorySlot = function(self, slot_key)
	if not self.inventory or not self.inventory.useSlot then
		return false
	end
	return self.inventory:useSlot(slot_key, self)
end

Storage.dropInventorySlot = function(self, slot_key)
	if not self.inventory or not self.inventory.dropWorldItemFromSlot then
		return false
	end
	return self.inventory:dropWorldItemFromSlot(slot_key, self)
end

return Storage
