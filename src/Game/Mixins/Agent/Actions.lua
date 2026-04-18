local Ecology = include("src/Game/Ecology/Ecology.lua")

local AgentActions = {}

local function shallow_action_data_equal(a, b)
	if a == b then
		return true
	end
	if type(a) ~= "table" or type(b) ~= "table" then
		return a == b
	end
	for key, value in pairs(a) do
		if b[key] ~= value then
			return false
		end
	end
	for key, value in pairs(b) do
		if a[key] ~= value then
			return false
		end
	end
	return true
end

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
	local flee_target = self:getPerceivedTargetForAction(Ecology.Actions.Flee)
	if flee_target and self.flee_radius and self:shouldFleeTarget(flee_target, self.flee_radius) then
		return Ecology.Actions.Flee, { target = flee_target }
	end
	return self.default_action, {}
end

AgentActions.updateActionPlan = function(self)
	if self.isPlayerControlled and self:isPlayerControlled() then
		return
	end
	if self.action_plan_timer and not self.action_plan_timer:hasElapsed(self.action_plan_interval_ms or 250) then
		return
	end
	local action, data = self:selectAction()
	action = action or self.default_action or Ecology.Actions.Idle
	local current_action = self.actions and self.actions.action or nil
	local current_data = self.actions and self.actions.data or nil
	if current_action == action and shallow_action_data_equal(current_data, data or {}) then
		return
	end
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
