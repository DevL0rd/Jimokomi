local Vector = include("src/classes/Vector.lua")
local Ecology = include("src/classes/Ecology.lua")
local WorldObject = include("src/primitives/WorldObject.lua")
local Creature = include("src/primitives/Creature/Creature.lua")
local WorldQuery = include("src/classes/WorldQuery.lua")
local GroundPatrolBehavior = include("src/classes/behaviors/GroundPatrolBehavior.lua")

local Worm = WorldObject:new({
	_type = "Worm",
	shape = {
		kind = "circle",
		r = 3,
	},
	edible = true,
	faction = Ecology.Factions.Wildlife,
	diet = Ecology.Diets.Herbivore,
	temperament = Ecology.Temperaments.Neutral,
	default_action = Ecology.Actions.Patrol,
	faction_actions = {
		player = Ecology.Actions.Watch,
	},
	initial_state = "patrol",
	move_accel = 70,
	max_speed = 18,
	eat_radius_padding = 2,
	spawn_rule = WorldQuery.rules.randomSurface({
		scope = "map",
		margin = 16,
		padding = 24,
		attempts = 24,
	}),
	init = function(self)
		local assets = self.layer and self.layer.engine and self.layer.engine.assets or nil
		if assets then
			assets:applyEntityConfig(self)
		end
		self.move_accel = self.move_accel or 70
		self.max_speed = self.max_speed or 18
		WorldObject.init(self)
		Creature.init(self)
		self.ignore_gravity = false
		self.ignore_friction = false
		self.ground_patrol = GroundPatrolBehavior:new({
			owner = self,
			move_accel = self.move_accel,
			max_speed = self.max_speed,
		})
		self:playVisual("crawl")
	end,
	isSolidAt = function(self, x, y)
		local world = self:getWorld()
		if not world then
			return false
		end
		return world:isSolidAt(x, y)
	end,
	isOnGround = function(self)
		local radius = self:getRadius()
		return self:isSolidAt(self.pos.x, self.pos.y + radius + 1)
	end,
	isBlockedAhead = function(self)
		local radius = self:getRadius()
		local probe_x = self.pos.x + self.direction * (radius + 2)
		return self:isSolidAt(probe_x, self.pos.y)
	end,
	hasGroundAhead = function(self)
		local radius = self:getRadius()
		local probe_x = self.pos.x + self.direction * (radius + 4)
		local probe_y = self.pos.y + radius + 2
		return self:isSolidAt(probe_x, probe_y)
	end,
	reverseDirection = function(self)
		self.ground_patrol:reverseDirection()
	end,
	onGroundPatrolDirectionChanged = function(self, behavior, direction)
		self:setVisualFlip(direction > 0, false)
	end,
	afterSpawn = function(self)
		self:setState("patrol")
		self.ground_patrol:afterSpawn()
	end,
	selectAction = function(self)
		return Ecology.Actions.Patrol, {}
	end,
	update_agent = function(self)
		self.ground_patrol:update()
	end,
	update = function(self)
		if Creature.update(self) == false then
			return
		end
		WorldObject.update(self)
	end,
	on_collision = function(self, ent, vector)
		if vector.x ~= 0 then
			self:reverseDirection()
		end
		Creature.on_collision(self, ent, vector)
	end
})

return Worm
