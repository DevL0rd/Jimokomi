local Class = include("src/Engine/Core/Class.lua")

local FrameClock = Class:new({
	_type = "FrameClock",
	last_time = 0,
	delta = 0,

	start = function(self)
		self.last_time = time()
		self.delta = 0
	end,

	tick = function(self)
		local now = time()
		self.delta = now - self.last_time
		self.last_time = now
		return self.delta
	end,
})

return FrameClock
