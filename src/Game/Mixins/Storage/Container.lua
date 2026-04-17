local Class = include("src/Engine/Core/Class.lua")

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

local function clamp_stack_limit(stack, limit)
	local stack_limit = stack and stack.max_stack_size or 1
	if limit == nil then
		return stack_limit
	end
	return min(limit, stack_limit)
end

local Container = Class:new({
	_type = "Container",
	owner = nil,
	slot_count = 0,
	slots = nil,
	slot_keys = nil,
	accepts_tags = nil,
	slot_max_stack_size = nil,
	slot_limits = nil,

	init = function(self)
		self.slots = self.slots or {}
		self.slot_keys = self.slot_keys or {}
		self.slot_limits = self.slot_limits or {}
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

	getSlotStackLimit = function(self, slot_key, stack)
		local slot_limit = self.slot_limits and self.slot_limits[slot_key] or self.slot_max_stack_size
		return clamp_stack_limit(stack, slot_limit)
	end,

	canAcceptStack = function(self, stack, slot_key)
		if not stack then
			return false
		end
		if slot_key ~= nil and self.slot_keys and #self.slot_keys > 0 then
			local has_key = false
			for _, key in pairs(self.slot_keys) do
				if key == slot_key then
					has_key = true
					break
				end
			end
			if not has_key then
				return false
			end
		end
		if self.accepts_tags == nil then
			return true
		end
		local tags = stack.item_tags or {}
		for _, accepted_tag in pairs(self.accepts_tags) do
			if tags[accepted_tag] then
				return true
			end
		end
		return false
	end,

	canAcceptItem = function(self, item, slot_key)
		if not item or not item.toItemStack then
			return false
		end
		return self:canAcceptStack(item:toItemStack(1), slot_key)
	end,

	findMergeSlot = function(self, stack, preferred_slot_key)
		if preferred_slot_key ~= nil then
			local current = self.slots[preferred_slot_key]
			local limit = self:getSlotStackLimit(preferred_slot_key, stack)
			if current and current.item_id == stack.item_id and current.amount < limit then
				return preferred_slot_key
			end
			return nil
		end

		for _, slot_key in pairs(self:getSlotKeys()) do
			local current = self.slots[slot_key]
			local limit = self:getSlotStackLimit(slot_key, stack)
			if current and current.item_id == stack.item_id and current.amount < limit then
				return slot_key
			end
		end
		return nil
	end,

	findEmptySlot = function(self, preferred_slot_key)
		if preferred_slot_key ~= nil then
			if self.slots[preferred_slot_key] == nil then
				return preferred_slot_key
			end
			return nil
		end

		for _, slot_key in pairs(self:getSlotKeys()) do
			if self.slots[slot_key] == nil then
				return slot_key
			end
		end
		return nil
	end,

	addStack = function(self, stack, preferred_slot_key)
		if not stack or stack.amount <= 0 then
			return 0
		end
		if not self:canAcceptStack(stack, preferred_slot_key) then
			return 0
		end

		local remaining = stack.amount

		while remaining > 0 do
			local merge_slot = self:findMergeSlot(stack, preferred_slot_key)
			if merge_slot then
				local current = self.slots[merge_slot]
				local room = self:getSlotStackLimit(merge_slot, stack) - current.amount
				local moved = min(room, remaining)
				current.amount += moved
				remaining -= moved
			else
				local empty_slot = self:findEmptySlot(preferred_slot_key)
				if empty_slot == nil then
					break
				end
				local next_stack = clone_table(stack)
				next_stack.max_stack_size = self:getSlotStackLimit(empty_slot, stack)
				next_stack.amount = min(next_stack.max_stack_size, remaining)
				self.slots[empty_slot] = next_stack
				remaining -= next_stack.amount
			end

			if preferred_slot_key ~= nil then
				if remaining > 0 then
					break
				end
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

	transferTo = function(self, target, from_slot_key, amount, to_slot_key)
		if not target or not target.addStack then
			return 0
		end
		local source_stack = self.slots[from_slot_key]
		if not source_stack then
			return 0
		end
		local moved_stack = clone_table(source_stack)
		moved_stack.amount = min(amount or source_stack.amount, source_stack.amount)
		local moved = target:addStack(moved_stack, to_slot_key)
		if moved <= 0 then
			return 0
		end
		self:removeAmount(from_slot_key, moved)
		return moved
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
			slot_max_stack_size = self.slot_max_stack_size,
			slot_limits = clone_table(self.slot_limits or {}),
			slots = slots,
		}
	end,

	applySnapshot = function(self, snapshot)
		self.slot_count = snapshot.slot_count or self.slot_count
		self.slot_keys = clone_table(snapshot.slot_keys or self.slot_keys or {})
		self.slot_max_stack_size = snapshot.slot_max_stack_size or self.slot_max_stack_size
		self.slot_limits = clone_table(snapshot.slot_limits or self.slot_limits or {})
		self.slots = {}
		for key, stack in pairs(snapshot.slots or {}) do
			self.slots[key] = stack and clone_table(stack) or nil
		end
	end,
})

return Container
