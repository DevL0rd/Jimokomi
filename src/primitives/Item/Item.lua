local VisualOwner = include("src/primitives/VisualOwner.lua")
local ItemStacks = include("src/primitives/Item/item/ItemStacks.lua")
local ItemInteractions = include("src/primitives/Item/item/ItemInteractions.lua")
local ItemVisuals = include("src/primitives/Item/item/ItemVisuals.lua")

local Item = {}

Item.mixin = function(self)
	if self._item_mixin_applied then
		return
	end

	self._item_mixin_applied = true

	for key, value in pairs(Item) do
		if key ~= "mixin" and key ~= "init" and type(value) == "function" and self[key] == nil then
			self[key] = value
		end
	end
end

Item.init = function(self)
	Item.mixin(self)
	VisualOwner.init(self)
	self.item_id = self.item_id or self._type or "Item"
	self.display_name = self.display_name or self.item_id
	self.max_stack_size = self.max_stack_size or 1
	self.stack_size = self.stack_size or 1
	self.can_pickup = self.can_pickup ~= false
	self.can_use = self.can_use or false
	self.can_place = self.can_place or false
	self.can_drop = self.can_drop ~= false
	self.can_consume = self.can_consume or false
	self.pickup_action_name = self.pickup_action_name or "pickup"
	self.use_action_name = self.use_action_name or "use"
	self.place_action_name = self.place_action_name or "place"
	self.drop_action_name = self.drop_action_name or "drop"
	self.consume_action_name = self.consume_action_name or "consume"
	self.item_tags = self.item_tags or {}
	self.equipment_slot = self.equipment_slot or nil
end

Item.update = function(self)
	return true
end

Item.getStackSize = ItemStacks.getStackSize
Item.getMaxStackSize = ItemStacks.getMaxStackSize
Item.isStackFull = ItemStacks.isStackFull
Item.canStackWith = ItemStacks.canStackWith
Item.addToStack = ItemStacks.addToStack
Item.removeFromStack = ItemStacks.removeFromStack
Item.getAvailableInteractions = ItemInteractions.getAvailableInteractions
Item.toItemStack = ItemStacks.toItemStack
Item.playVisual = ItemVisuals.playVisual
Item.clearVisual = ItemVisuals.clearVisual
Item.onPickedUp = ItemInteractions.onPickedUp
Item.onUsed = ItemInteractions.onUsed
Item.onPlaced = ItemInteractions.onPlaced
Item.onDropped = ItemInteractions.onDropped
Item.pickup = ItemInteractions.pickup
Item.use = ItemInteractions.use
Item.place = ItemInteractions.place
Item.drop = ItemInteractions.drop
Item.on_collision = ItemInteractions.on_collision

return Item
