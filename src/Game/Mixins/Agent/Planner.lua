local Class = include("src/Engine/Core/Class.lua")

local Planner = Class:new({
	_type = "Planner",
	owner = nil,
	action = "idle",
	previous_action = nil,
	data = nil,
	init = function(self)
		self.action = self.action or "idle"
		self.previous_action = nil
		self.data = self.data or {}
		self.action_started_at = time()
	end,
	setAction = function(self, action, data)
		data = data or {}
		if self.action == action then
			self.data = data
			return false
		end
		self.previous_action = self.action
		self.action = action
		self.data = data
		self.action_started_at = time()
		return true
	end,
	restart = function(self, data)
		if data ~= nil then
			self.data = data
		end
		self.action_started_at = time()
	end,
	isAction = function(self, action)
		return self.action == action
	end,
	elapsed = function(self)
		return (time() - self.action_started_at) * 1000
	end,
	hasElapsed = function(self, duration)
		return self:elapsed() >= duration
	end,
})

return Planner
