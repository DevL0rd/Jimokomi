local Class = include("src/Engine/Core/Class.lua")

local FrameClock = Class:new({
	_type = "FrameClock",
	last_time = 0,
	delta = 0,
	measured_delta = 0,
	fallback_delta = 1 / 60,
	fixed_delta = 1 / 60,

	start = function(self)
		self.last_time = time()
		self.measured_delta = self.fallback_delta
		self.delta = self.fixed_delta or self.fallback_delta
	end,

	tick = function(self)
		local now = time()
		local delta = now - self.last_time
		self.last_time = now
		if delta <= 0 then
			delta = self.measured_delta and self.measured_delta > 0 and self.measured_delta or self.fallback_delta
		end
		self.measured_delta = delta
		self.delta = self.fixed_delta or self.fallback_delta
		return self.delta
	end,
})

return FrameClock
