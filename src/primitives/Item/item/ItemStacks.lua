local ItemStacks = {}

ItemStacks.getStackSize = function(self)
	return self.stack_size or 0
end

ItemStacks.getMaxStackSize = function(self)
	return self.max_stack_size or 1
end

ItemStacks.isStackFull = function(self)
	return self:getStackSize() >= self:getMaxStackSize()
end

ItemStacks.canStackWith = function(self, other)
	if not other then
		return false
	end
	return self.item_id == other.item_id and not self:isStackFull()
end

ItemStacks.addToStack = function(self, amount)
	amount = amount or 1
	local next_size = mid(0, self:getStackSize() + amount, self:getMaxStackSize())
	local added = next_size - self:getStackSize()
	self.stack_size = next_size
	return added
end

ItemStacks.removeFromStack = function(self, amount)
	amount = amount or 1
	local next_size = max(0, self:getStackSize() - amount)
	local removed = self:getStackSize() - next_size
	self.stack_size = next_size
	return removed
end

ItemStacks.toItemStack = function(self, amount)
	amount = amount or self:getStackSize()
	return {
		item_id = self.item_id,
		display_name = self.display_name,
		amount = amount,
		max_stack_size = self:getMaxStackSize(),
		can_use = self.can_use == true,
		can_place = self.can_place == true,
		can_drop = self.can_drop ~= false,
		can_consume = self.can_consume == true,
		consume_action_name = self.consume_action_name,
		use_action_name = self.use_action_name,
		place_action_name = self.place_action_name,
		drop_action_name = self.drop_action_name,
		item_tags = self.item_tags,
		equipment_slot = self.equipment_slot,
	}
end

return ItemStacks
