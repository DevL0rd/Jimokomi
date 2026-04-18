local State = include("src/Game/Mixins/Agent/State.lua")
local Planner = include("src/Game/Mixins/Agent/Planner.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local AgentRelations = include("src/Game/Mixins/Agent/Relations.lua")
local AgentActions = include("src/Game/Mixins/Agent/Actions.lua")
local AgentSpawning = include("src/Game/Mixins/Agent/Spawning.lua")
local AgentPerception = include("src/Game/Mixins/Agent/Perception.lua")

local Agent = {}
Agent.ControlModes = {
	Player = 1,
	Autonomous = 2,
}

Agent.mixin = function(self)
	if self._agent_mixin_applied then
		return
	end

	self._agent_mixin_applied = true

	for key, value in pairs(Agent) do
		if key ~= "mixin" and key ~= "init" and type(value) == "function" and self[key] == nil then
			self[key] = value
		end
	end
end

Agent.init = function(self)
	Agent.mixin(self)
	self.ai = State:new({
		owner = self,
		state = self.initial_state or "idle"
	})
	self.faction = self.faction or Ecology.Factions.Wildlife
	self.diet = self.diet or Ecology.Diets.None
	self.temperament = self.temperament or Ecology.Temperaments.Neutral
	self.default_action = self.default_action or Ecology.getDefaultActionForTemperament(self.temperament)
	self.control_mode = self.control_mode or Agent.ControlModes.Autonomous
	self.actions = Planner:new({
		owner = self,
		action = self.initial_action or self.default_action
	})
	self.faction_actions = self.faction_actions or {}
	self.is_spawned = self.spawn_rule == nil
	self.action_plan_interval_ms = self.action_plan_interval_ms or 250
	self.action_plan_timer = self.action_plan_timer or Timer:new()
	self.action_plan_timer.start_time -= random_int(0, self.action_plan_interval_ms) / 1000
	self:initPerception()
end

Agent.isAutonomous = function(self)
	return self.control_mode ~= Agent.ControlModes.Player
end

Agent.isPlayerControlled = function(self)
	return self.control_mode == Agent.ControlModes.Player
end

Agent.setControlMode = function(self, control_mode)
	self.control_mode = control_mode or Agent.ControlModes.Player
	return self.control_mode
end

Agent.getFaction = AgentRelations.getFaction
Agent.getDiet = AgentRelations.getDiet
Agent.getTemperament = AgentRelations.getTemperament
Agent.getActionToward = AgentRelations.getActionToward
Agent.getDispositionToward = AgentRelations.getDispositionToward
Agent.getCurrentAction = AgentActions.getCurrentAction
Agent.setAction = AgentActions.setAction
Agent.isAction = AgentActions.isAction
Agent.getActionElapsed = AgentActions.getActionElapsed
Agent.restartAction = AgentActions.restartAction
Agent.isTimidToward = AgentRelations.isTimidToward
Agent.isNeutralToward = AgentRelations.isNeutralToward
Agent.isAggressiveToward = AgentRelations.isAggressiveToward
Agent.isDefensiveToward = AgentRelations.isDefensiveToward
Agent.getPlayer = AgentRelations.getPlayer
Agent.getWorld = AgentRelations.getWorld
Agent.getVectorTo = AgentRelations.getVectorTo
Agent.getDistanceTo = AgentRelations.getDistanceTo
Agent.isTargetWithinRange = AgentRelations.isTargetWithinRange
Agent.getAwayVector = AgentRelations.getAwayVector
Agent.shouldFleeTarget = AgentRelations.shouldFleeTarget
Agent.shouldFleePlayer = AgentRelations.shouldFleePlayer
Agent.initPerception = AgentPerception.initPerception
Agent.canPerceiveFaction = AgentPerception.canPerceiveFaction
Agent.canSeeTarget = AgentPerception.canSeeTarget
Agent.getSeenTarget = AgentPerception.getSeenTarget
Agent.getHeardSound = AgentPerception.getHeardSound
Agent.updatePerception = AgentPerception.updatePerception
Agent.getPerceivedTarget = AgentPerception.getPerceivedTarget
Agent.getPerceivedTargetForAction = AgentPerception.getPerceivedTargetForAction
Agent.getFacingVector = AgentPerception.getFacingVector
Agent.isOnGroundForSound = AgentPerception.isOnGroundForSound
Agent.emitSound = AgentPerception.emitSound
Agent.updateSounding = AgentPerception.updateSounding
Agent.selectAction = AgentActions.selectAction
Agent.updateActionPlan = AgentActions.updateActionPlan
Agent.getSpawnRadius = AgentSpawning.getSpawnRadius
Agent.getEatRadius = AgentSpawning.getEatRadius
Agent.setState = AgentActions.setState
Agent.isState = AgentActions.isState
Agent.getStateElapsed = AgentActions.getStateElapsed
Agent.restartState = AgentActions.restartState
Agent.setSpawnPosition = AgentSpawning.setSpawnPosition
Agent.afterSpawn = AgentSpawning.afterSpawn
Agent.onEaten = AgentSpawning.onEaten
Agent.respawn = AgentSpawning.respawn
Agent.trySpawn = AgentSpawning.trySpawn
Agent.isEatenByPlayer = AgentSpawning.isEatenByPlayer
Agent.update = AgentSpawning.update

return Agent
