local Agent = include("src/Game/Mixins/Agent.lua")
local VisualOwner = include("src/Engine/Mixins/VisualOwner.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")

local Creature = {}

Creature.mixin = function(self)
	if self._creature_mixin_applied then
		return
	end

	self._creature_mixin_applied = true

	for key, value in pairs(Creature) do
		if key ~= "mixin" and key ~= "init" and type(value) == "function" and self[key] == nil then
			self[key] = value
		end
	end
end

Creature.init = function(self)
	Creature.mixin(self)
	Agent.init(self)
	VisualOwner.init(self)
end

Creature.update = function(self)
	return Agent.update(self)
end

Creature.playVisual = function(self, state, overrides)
	return VisualOwner.playVisual(self, state, overrides)
end

Creature.clearVisual = function(self)
	VisualOwner.clearVisual(self)
end

Creature.on_collision = function(self, ent, vector)
	WorldObject.on_collision(self, ent, vector)
end

return Creature
