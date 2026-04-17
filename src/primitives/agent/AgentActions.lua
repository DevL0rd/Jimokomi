local Ecology = include("src/classes/Ecology.lua")

local AgentActions = {}

AgentActions.getCurrentAction = function(self)
	return self.actions.action
end

AgentActions.setAction = function(self, action, data)
	return self.actions:setAction(action, data)
end

AgentActions.isAction = function(self, action)
	return self.actions:isAction(action)
end

AgentActions.getActionElapsed = function(self)
	return self.actions:elapsed()
end

AgentActions.restartAction = function(self, data)
	self.actions:restart(data)
end

AgentActions.selectAction = function(self)
	local player = self:getPlayer()
	if player and self.flee_radius and self:shouldFleeTarget(player, self.flee_radius) then
		return Ecology.Actions.Flee, { target = player }
	end
	return self.default_action, {}
end

AgentActions.updateActionPlan = function(self)
	local action, data = self:selectAction()
	action = action or self.default_action or Ecology.Actions.Idle
	self:setAction(action, data)
end

AgentActions.setState = function(self, state, data)
	return self.ai:setState(state, data)
end

AgentActions.isState = function(self, state)
	return self.ai:isState(state)
end

AgentActions.getStateElapsed = function(self)
	return self.ai:elapsed()
end

AgentActions.restartState = function(self, data)
	self.ai:restart(data)
end

return AgentActions
