local Class = include("src/Engine/Core/Class.lua")

local Behavior = Class:new({
	_type = "Behavior",
	owner = nil,

	notify = function(self, method_name, ...)
		if self.owner and self.owner[method_name] then
			return self.owner[method_name](self.owner, self, ...)
		end
		return nil
	end,

	getWorld = function(self)
		if not self.owner or not self.owner.getWorld then
			return nil
		end
		return self.owner:getWorld()
	end,

	getPlayer = function(self)
		if not self.owner or not self.owner.getPlayer then
			return nil
		end
		return self.owner:getPlayer()
	end,
})

return Behavior
