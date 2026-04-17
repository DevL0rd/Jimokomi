local Class = include("src/Engine/Core/Class.lua")

local State = Class:new({
	_type = "State",
	owner = nil,
	state = "idle",
	previous_state = nil,
	data = nil,

	init = function(self)
		self.state = self.state or "idle"
		self.previous_state = nil
		self.data = self.data or {}
		self.state_started_at = time()
	end,

	setState = function(self, state, data)
		data = data or {}
		if self.state == state then
			self.data = data
			return false
		end
		self.previous_state = self.state
		self.state = state
		self.data = data
		self.state_started_at = time()
		return true
	end,

	restart = function(self, data)
		if data ~= nil then
			self.data = data
		end
		self.state_started_at = time()
	end,

	isState = function(self, state)
		return self.state == state
	end,

	elapsed = function(self)
		return (time() - self.state_started_at) * 1000
	end,

	hasElapsed = function(self, duration)
		return self:elapsed() >= duration
	end,
})

return State
