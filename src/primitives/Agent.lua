local StateMachine = include("src/classes/StateMachine.lua")
local ActionPlanner = include("src/classes/ActionPlanner.lua")
local Ecology = include("src/classes/Ecology.lua")
local AgentRelations = include("src/primitives/agent/AgentRelations.lua")
local AgentActions = include("src/primitives/agent/AgentActions.lua")
local AgentSpawning = include("src/primitives/agent/AgentSpawning.lua")

local Agent = {}

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
	self.ai = StateMachine:new({
		owner = self,
		state = self.initial_state or "idle"
	})
	self.faction = self.faction or Ecology.Factions.Wildlife
	self.diet = self.diet or Ecology.Diets.None
	self.temperament = self.temperament or Ecology.Temperaments.Neutral
	self.default_action = self.default_action or Ecology.getDefaultActionForTemperament(self.temperament)
	self.actions = ActionPlanner:new({
		owner = self,
		action = self.initial_action or self.default_action
	})
	self.faction_actions = self.faction_actions or {}
	self.is_spawned = self.spawn_rule == nil
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
Agent.getSpawner = AgentRelations.getSpawner
Agent.getVectorTo = AgentRelations.getVectorTo
Agent.getDistanceTo = AgentRelations.getDistanceTo
Agent.isTargetWithinRange = AgentRelations.isTargetWithinRange
Agent.getAwayVector = AgentRelations.getAwayVector
Agent.shouldFleeTarget = AgentRelations.shouldFleeTarget
Agent.shouldFleePlayer = AgentRelations.shouldFleePlayer
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
