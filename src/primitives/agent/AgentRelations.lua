local Ecology = include("src/classes/Ecology.lua")
local Vector = include("src/classes/Vector.lua")

local AgentRelations = {}

AgentRelations.getFaction = function(self)
	return self.faction or Ecology.Factions.Wildlife
end

AgentRelations.getDiet = function(self)
	return self.diet or Ecology.Diets.None
end

AgentRelations.getTemperament = function(self)
	return self.temperament or Ecology.Temperaments.Neutral
end

AgentRelations.getActionToward = function(self, target)
	if not target then
		return self.default_action or Ecology.Actions.Watch
	end

	local target_faction = Ecology.Factions.Wildlife
	if target.getFaction then
		target_faction = target:getFaction()
	elseif target.faction then
		target_faction = target.faction
	end

	if self.faction_actions and self.faction_actions[target_faction] then
		return self.faction_actions[target_faction]
	end

	return self.default_action or Ecology.Actions.Watch
end

AgentRelations.getDispositionToward = function(self, target)
	local action = self:getActionToward(target)

	if action == Ecology.Actions.Flee then
		return Ecology.Temperaments.Timid
	end
	if action == Ecology.Actions.Defend or action == Ecology.Actions.Warn then
		return Ecology.Temperaments.Defensive
	end
	if action == Ecology.Actions.Chase or action == Ecology.Actions.Hunt then
		return Ecology.Temperaments.Aggressive
	end
	if action == Ecology.Actions.Ignore then
		return Ecology.Temperaments.Passive
	end

	return Ecology.Temperaments.Neutral
end

AgentRelations.isTimidToward = function(self, target)
	return self:getDispositionToward(target) == Ecology.Temperaments.Timid
end

AgentRelations.isNeutralToward = function(self, target)
	return self:getDispositionToward(target) == Ecology.Temperaments.Neutral
end

AgentRelations.isAggressiveToward = function(self, target)
	return self:getDispositionToward(target) == Ecology.Temperaments.Aggressive
end

AgentRelations.isDefensiveToward = function(self, target)
	return self:getDispositionToward(target) == Ecology.Temperaments.Defensive
end

AgentRelations.getPlayer = function(self)
	if not self.layer or not self.layer.getPlayer then
		return nil
	end
	return self.layer:getPlayer()
end

AgentRelations.getWorld = function(self)
	if not self.layer then
		return nil
	end
	return self.layer.world
end

AgentRelations.getSpawner = function(self)
	if not self.layer then
		return nil
	end
	return self.layer.spawner
end

AgentRelations.getVectorTo = function(self, target)
	if not target then
		return nil
	end

	return Vector:new({
		x = target.pos.x - self.pos.x,
		y = target.pos.y - self.pos.y,
	})
end

AgentRelations.getDistanceTo = function(self, target)
	local delta = self:getVectorTo(target)
	if not delta then
		return nil
	end
	return sqrt(delta.x * delta.x + delta.y * delta.y)
end

AgentRelations.isTargetWithinRange = function(self, target, range)
	local distance = self:getDistanceTo(target)
	if distance == nil then
		return false
	end
	return distance <= range
end

AgentRelations.getAwayVector = function(self, target)
	local delta = self:getVectorTo(target)
	if not delta then
		return nil
	end

	local dist = sqrt(delta.x * delta.x + delta.y * delta.y)
	if dist <= 0 then
		return Vector:new({ x = 0, y = -1 })
	end

	return Vector:new({
		x = -delta.x / dist,
		y = -delta.y / dist,
	})
end

AgentRelations.shouldFleeTarget = function(self, target, radius)
	if not target or not radius then
		return false
	end
	if self:getActionToward(target) ~= Ecology.Actions.Flee then
		return false
	end
	return self:isTargetWithinRange(target, radius)
end

AgentRelations.shouldFleePlayer = function(self, radius)
	return self:shouldFleeTarget(self:getPlayer(), radius)
end

return AgentRelations
