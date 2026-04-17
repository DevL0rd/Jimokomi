local Vector = include("src/Engine/Math/Vector.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Creature = include("src/Game/Mixins/Creature.lua")
local GroundPatrolBehavior = include("src/Game/Locomotion/Autonomous/GroundPatrol.lua")

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
	visual_definitions = {
		crawl = {
			shape = { kind = "rect", w = 6, h = 1 },
			offset = { x = 0, y = 6 },
			sprite = 0x1b,
			end_sprite = 0x1c,
			speed = 6,
		},
	},
	eat_radius_padding = 2,
	spawn_rule = {
		kind = "surface_random",
		scope = "map",
		margin = 16,
		padding = 24,
		attempts = 24,
	},
	init = function(self)
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
