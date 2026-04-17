local Container = include("src/Game/Mixins/Storage/Container.lua")
local Vector = include("src/Engine/Math/Vector.lua")

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
		if not owner or not owner.layer or not owner.pos then
			return false
		end

		local direction_offset_x = 0
		if owner.direction == 2 then
			direction_offset_x = -18
		elseif owner.direction == 3 then
			direction_offset_x = 18
		end

		local ItemClass = include(stack.spawn_item_path)
		ItemClass:new({
			layer = owner.layer,
			pos = Vector:new({
				x = owner.pos.x + direction_offset_x,
				y = owner.pos.y,
			}),
			stack_size = 1,
		})
		self:removeAmount(slot_key, 1)
		return true
	end,
})

return Inventory
