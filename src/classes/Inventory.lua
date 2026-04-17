local Container = include("src/classes/Container.lua")

local Inventory = Container:new({
	_type = "Inventory",

	pickupWorldItem = function(self, item)
		if not item or not item.toItemStack then
			return 0
		end
		if not self:canAcceptItem(item) then
			return 0
		end

		local added = self:addStack(item:toItemStack(item:getStackSize()))
		if added <= 0 then
			return 0
		end

		if added >= item:getStackSize() then
			item:destroy()
		else
			item:removeFromStack(added)
		end

		return added
	end,
})

return Inventory
