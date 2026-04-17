local Container = include("src/Game/Mixins/Storage/Container.lua")
local Vector = include("src/Engine/Math/Vector.lua")

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

local function get_drop_position(owner)
	if not owner or not owner.layer or not owner.pos then
		return nil
	end

	local direction_offset_x = 0
	if owner.direction == 2 then
		direction_offset_x = -18
	elseif owner.direction == 3 then
		direction_offset_x = 18
	end

	return Vector:new({
		x = owner.pos.x + direction_offset_x,
		y = owner.pos.y,
	})
end

local function instantiate_world_item_from_stack(stack, owner, drop_pos)
	if not stack or not stack.spawn_item_path or not owner or not owner.layer then
		return nil
	end

	local ItemClass = include(stack.spawn_item_path)
	local item = ItemClass:new({
		layer = owner.layer,
		pos = drop_pos,
		stack_size = 1,
	})

	if stack.captured_snapshot and item.applySnapshot then
		local snapshot = clone_table(stack.captured_snapshot)
		snapshot.object_id = nil
		if snapshot.transform then
			snapshot.transform.attachment = nil
			snapshot.transform.local_position = {
				x = drop_pos.x,
				y = drop_pos.y,
			}
			snapshot.transform.position = {
				x = drop_pos.x,
				y = drop_pos.y,
			}
		end
		item:applySnapshot(snapshot)
	end

	if item.stack_size ~= nil then
		item.stack_size = 1
	end

	return item
end

local Inventory = Container:new({
	_type = "Inventory",

	pickupWorldItem = function(self, item, slot_key)
		if not item or not item.toItemStack then
			return 0
		end
		if not self:canAcceptItem(item, slot_key) then
			return 0
		end

		local added = self:addStack(item:toItemStack(item:getStackSize()), slot_key)
		if added <= 0 then
			return 0
		end

		return added
	end,

	useSlot = function(self, slot_key, user)
		local stack = self:getSlot(slot_key)
		if not stack or not stack.can_use then
			return false
		end

		local did_use = false
		if stack.heal_amount and stack.heal_amount > 0 and user and user.heal then
			user:heal(stack.heal_amount)
			did_use = true
		end

		if not did_use then
			return false
		end

		self:removeAmount(slot_key, 1)
		return true
	end,

	dropWorldItemFromSlot = function(self, slot_key, owner)
		local stack = self:getSlot(slot_key)
		if not stack or not stack.can_drop or not stack.spawn_item_path then
			return false
		end
		local drop_pos = get_drop_position(owner)
		if not drop_pos then
			return false
		end

		local item = instantiate_world_item_from_stack(stack, owner, drop_pos)
		if not item then
			return false
		end
		self:removeAmount(slot_key, 1)
		return true
	end,
})

return Inventory
