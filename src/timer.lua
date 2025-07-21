Timer = Class:new({
	_type = "Timer",
	init = function(self)
		self:reset()
	end,
	elapsed = function(self)
		return (time() - self.start_time) * 1000
	end,
	hasElapsed = function(self, targetTime, shouldReset)
		shouldReset = shouldReset or true
		local res = self:elapsed() >= targetTime
		if res and shouldReset then
			self:reset()
		end
		return res
	end,
	reset = function(self)
		self.start_time = time()
	end
})
