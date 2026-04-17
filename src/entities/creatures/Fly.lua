local Ecology = include("src/classes/Ecology.lua")
local WorldObject = include("src/primitives/WorldObject.lua")
local Creature = include("src/primitives/Creature/Creature.lua")
local WorldQuery = include("src/classes/WorldQuery.lua")
local FlightBehavior = include("src/classes/behaviors/FlightBehavior.lua")

local Fly = WorldObject:new({
	_type = "Fly",
	shape = {
		kind = "circle",
		r = 2,
	},
	edible = true,
	ignore_physics = true,
	ignore_gravity = true,
	ignore_friction = true,
	faction = Ecology.Factions.Wildlife,
	diet = Ecology.Diets.Omnivore,
	temperament = Ecology.Temperaments.Timid,
	default_action = Ecology.Actions.Wander,
	faction_actions = {
		player = Ecology.Actions.Flee,
	},
	initial_state = "flying",
	eat_radius_padding = 2,
	spawn_rule = WorldQuery.rules.randomMap({
		scope = "map",
		margin = 16,
		padding = 24,
		attempts = 24,
	}),
	landing_rule = WorldQuery.rules.randomSurface({
		margin = 16,
		attempts = 48,
	}),

	init = function(self)
		local assets = self.layer and self.layer.engine and self.layer.engine.assets or nil
		if assets then
			assets:applyEntityConfig(self)
		end
		self.flight_duration_min = self.flight_duration_min or 4800
		self.flight_duration_max = self.flight_duration_max or 9600
		self.landed_duration_min = self.landed_duration_min or 2400
		self.landed_duration_max = self.landed_duration_max or 5200
		self.landing_speed = self.landing_speed or 85
		self.explore_speed = self.explore_speed or 32
		self.landing_radius_x = self.landing_radius_x or 240
		self.landing_radius_y = self.landing_radius_y or 160
		self.flee_radius = self.flee_radius or 56
		self.flee_speed = self.flee_speed or 120
		self.flee_anchor_distance = self.flee_anchor_distance or 96
		self.explore_retarget_min = self.explore_retarget_min or 1200
		self.explore_retarget_max = self.explore_retarget_max or 3200
		WorldObject.init(self)
		Creature.init(self)
		self.flight = FlightBehavior:new({
			owner = self,
			explore_speed = self.explore_speed,
			landing_speed = self.landing_speed,
			flight_duration_min = self.flight_duration_min,
			flight_duration_max = self.flight_duration_max,
			landed_duration_min = self.landed_duration_min,
			landed_duration_max = self.landed_duration_max,
			explore_retarget_min = self.explore_retarget_min,
			explore_retarget_max = self.explore_retarget_max,
			flee_speed = self.flee_speed,
			flee_anchor_distance = self.flee_anchor_distance,
			explore_rule = self.spawn_rule,
			landing_rule = self.landing_rule,
			landing_radius_x = self.landing_radius_x,
			landing_radius_y = self.landing_radius_y,
		})
		self:playVisual("flying")
	end,

	onFlightModeChanged = function(self, behavior, mode)
		if mode == "landed" then
			self:playVisual("landed")
		else
			self:playVisual("flying")
		end
	end,

	onFlightFacingChanged = function(self, behavior, facing_right)
		self:setVisualFlip(facing_right, false)
	end,

	afterSpawn = function(self)
		self.flight:afterSpawn()
	end,

	selectAction = function(self)
		local player = self:getPlayer()
		if player and self:shouldFleeTarget(player, self.flee_radius) then
			return Ecology.Actions.Flee, { target = player }
		end
		if self:isState("landing") or self:isState("landed") then
			return Ecology.Actions.Perch, { target = self.flight.landing_target }
		end
		return Ecology.Actions.Wander, { target = self.flight.explore_target }
	end,

	update_agent = function(self)
		self.flight:update()
	end,

	update = function(self)
		if Creature.update(self) == false then
			return
		end
		WorldObject.update(self)
	end,
})

return Fly
