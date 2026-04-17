local Class = include("src/classes/Class.lua")

local function clone_table(source)
	if type(source) ~= "table" then
		return source
	end
	local copy = {}
	for key, value in pairs(source) do
		copy[key] = clone_table(value)
	end
	return copy
end

local Container = Class:new({
	_type = "Container",
	owner = nil,
	slot_count = 0,
	slots = nil,
	slot_keys = nil,
	accepts_tags = nil,

	init = function(self)
		self.slots = self.slots or {}
		self.slot_keys = self.slot_keys or {}
		if self.slot_count and self.slot_count > 0 and #self.slot_keys == 0 then
			for index = 1, self.slot_count do
				add(self.slot_keys, index)
			end
		end
	end,

	getSlotKeys = function(self)
		local keys = {}
		if self.slot_keys and #self.slot_keys > 0 then
			for _, key in pairs(self.slot_keys) do
				add(keys, key)
			end
			return keys
		end
		for key in pairs(self.slots) do
			add(keys, key)
		end
		return keys
	end,

	getSlot = function(self, slot_key)
		return self.slots[slot_key]
	end,

	setSlot = function(self, slot_key, stack)
		self.slots[slot_key] = stack
		return stack
	end,

	isEmptySlot = function(self, slot_key)
		return self.slots[slot_key] == nil
	end,

	canAcceptItem = function(self, item, slot_key)
		if not item then
			return false
		end
		if self.accepts_tags == nil then
			return true
		end
		local tags = item.item_tags or {}
		for _, accepted_tag in pairs(self.accepts_tags) do
			if tags[accepted_tag] then
				return true
			end
		end
		return false
	end,

	findMergeSlot = function(self, stack)
		for _, slot_key in pairs(self:getSlotKeys()) do
			local current = self.slots[slot_key]
			if current and current.item_id == stack.item_id and current.amount < current.max_stack_size then
				return slot_key
			end
		end
		return nil
	end,

	findEmptySlot = function(self)
		for _, slot_key in pairs(self:getSlotKeys()) do
			if self.slots[slot_key] == nil then
				return slot_key
			end
		end
		return nil
	end,

	addStack = function(self, stack)
		if not stack or stack.amount <= 0 then
			return 0
		end

		local remaining = stack.amount

		while remaining > 0 do
			local merge_slot = self:findMergeSlot(stack)
			if merge_slot then
				local current = self.slots[merge_slot]
				local room = current.max_stack_size - current.amount
				local moved = min(room, remaining)
				current.amount += moved
				remaining -= moved
			else
				local empty_slot = self:findEmptySlot()
				if empty_slot == nil then
					break
				end
				local next_stack = clone_table(stack)
				next_stack.amount = min(stack.max_stack_size, remaining)
				self.slots[empty_slot] = next_stack
				remaining -= next_stack.amount
			end
		end

		return stack.amount - remaining
	end,

	removeAmount = function(self, slot_key, amount)
		local stack = self.slots[slot_key]
		if not stack then
			return nil
		end
		amount = amount or stack.amount
		local removed = min(amount, stack.amount)
		local result = clone_table(stack)
		result.amount = removed
		stack.amount -= removed
		if stack.amount <= 0 then
			self.slots[slot_key] = nil
		end
		return result
	end,

	toSnapshot = function(self)
		local slots = {}
		for _, key in pairs(self:getSlotKeys()) do
			local stack = self.slots[key]
			slots[key] = stack and clone_table(stack) or nil
		end
		return {
			slot_count = self.slot_count,
			slot_keys = clone_table(self:getSlotKeys()),
			slots = slots,
		}
	end,

	applySnapshot = function(self, snapshot)
		self.slot_count = snapshot.slot_count or self.slot_count
		self.slot_keys = clone_table(snapshot.slot_keys or self.slot_keys or {})
		self.slots = {}
		for key, stack in pairs(snapshot.slots or {}) do
			self.slots[key] = stack and clone_table(stack) or nil
		end
	end,
})

return Container
