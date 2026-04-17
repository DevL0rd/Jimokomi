local Class = include("src/classes/Class.lua")
local Inventory = include("src/classes/Inventory.lua")
local Equipment = include("src/classes/Equipment.lua")

local PlayerLoadout = Class:new({
	_type = "PlayerLoadout",
	owner = nil,

	init = function(self)
		if not self.owner then
			return
		end

		self.owner.inventory = Inventory:new({
			owner = self.owner,
			slot_count = self.owner.inventory_size or 16,
		})
		self.owner.equipment = Equipment:new({
			owner = self.owner,
			slot_names = self.owner.equipment_slots or { "head", "mouth", "back", "feet", "hands" },
		})
	end,

	exportState = function(self)
		return {
			inventory = self.owner.inventory and self.owner.inventory:toSnapshot() or nil,
			equipment = self.owner.equipment and self.owner.equipment:toSnapshot() or nil,
		}
	end,

	importState = function(self, state)
		if not state then
			return
		end
		if state.inventory and self.owner.inventory then
			self.owner.inventory:applySnapshot(state.inventory)
		end
		if state.equipment and self.owner.equipment then
			self.owner.equipment:applySnapshot(state.equipment)
		end
	end,
})

return PlayerLoadout
