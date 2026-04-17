local VisualOwner = include("src/Engine/Mixins/VisualOwner.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local ItemStacks = include("src/Game/Mixins/Item/Stacks.lua")
local ItemInteractions = include("src/Game/Mixins/Item/Interactions.lua")
local ItemVisuals = include("src/Game/Mixins/Item/Visuals.lua")

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
	self.pickup_on_touch = self.pickup_on_touch == true
	self.touch_pickup_interval_ms = self.touch_pickup_interval_ms or 120
	self.touch_pickup_timer = self.touch_pickup_timer or Timer:new()
	self.pickup_radius_padding = self.pickup_radius_padding or 0
	self.destroy_on_pickup = self.destroy_on_pickup ~= false
	self.pickup_action_name = self.pickup_action_name or "pickup"
	self.use_action_name = self.use_action_name or "use"
	self.place_action_name = self.place_action_name or "place"
	self.drop_action_name = self.drop_action_name or "drop"
	self.item_tags = self.item_tags or {}
	self.equipment_slot = self.equipment_slot or nil
	self.heal_amount = self.heal_amount or 0
	self.spawn_item_path = self.spawn_item_path or nil
	self.inventory_sprite = self.inventory_sprite or nil
	self.replace_on_pickup = self.replace_on_pickup == true
end

Item.update = function(self)
	if self.pickup_on_touch and self.tryTouchPickup and self.touch_pickup_timer and self.touch_pickup_timer:hasElapsed(self.touch_pickup_interval_ms or 120) then
		self:tryTouchPickup()
	end
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
Item.spawnPickupReplacement = ItemInteractions.spawnPickupReplacement
Item.pickup = ItemInteractions.pickup
Item.use = ItemInteractions.use
Item.place = ItemInteractions.place
Item.drop = ItemInteractions.drop
Item.getPlayer = ItemInteractions.getPlayer
Item.getPickupRadius = ItemInteractions.getPickupRadius
Item.isTouchingCollector = ItemInteractions.isTouchingCollector
Item.tryTouchPickup = ItemInteractions.tryTouchPickup
Item.on_collision = ItemInteractions.on_collision

return Item
