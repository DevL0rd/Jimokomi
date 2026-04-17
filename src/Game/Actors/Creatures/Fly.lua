local Ecology = include("src/Game/Ecology/Ecology.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Creature = include("src/Game/Mixins/Creature.lua")
local Item = include("src/Game/Mixins/Item.lua")
local Flight = include("src/Game/Actors/Locomotion/Flight.lua")

local Fly = WorldObject:new({
	_type = "Fly",
	shape = {
		kind = "circle",
		r = 2,
	},
	edible = true,
	item_id = "fly",
	display_name = "Fly",
	ignore_physics = true,
	ignore_gravity = true,
	ignore_friction = true,
	ignore_collisions = true,
	can_pickup = true,
	can_use = true,
	can_drop = true,
	pickup_on_touch = true,
	pickup_radius_padding = 2,
	use_action_name = "eat",
	heal_amount = 1,
	max_stack_size = 1,
	spawn_item_path = "src/Game/Actors/Creatures/Fly.lua",
	replace_on_pickup = true,
	inventory_sprite = 0x19,
	faction = Ecology.Factions.Wildlife,
	diet = Ecology.Diets.Omnivore,
	temperament = Ecology.Temperaments.Timid,
	default_action = Ecology.Actions.Wander,
	vision_range = 84,
	hearing_range = 96,
	action_plan_interval_ms = 250,
	perception_interval_ms = 500,
	sound_update_interval_ms = 500,
	sound_idle_radius = 12,
	sound_move_radius = 36,
	sound_land_radius = 64,
	sound_move_interval_ms = 900,
	sound_idle_interval_ms = 2600,
	faction_actions = {
		player = Ecology.Actions.Flee,
	},
	initial_state = "flying",
	visual_definitions = {
		flying = {
			shape = { kind = "rect", w = 6, h = 3 },
			sprite = 0x19,
			end_sprite = 0x1a,
			speed = 14,
		},
		landed = {
			shape = { kind = "rect", w = 6, h = 3 },
			sprite = 24,
		},
	},
	eat_radius_padding = 2,
	spawn_rule = {
		kind = "map_random",
		scope = "map",
		margin = 16,
		padding = 24,
		attempts = 24,
	},
	landing_rule = {
		kind = "surface_random",
		margin = 16,
		attempts = 48,
	},

	init = function(self)
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
		Item.init(self)
		self.flight_locomotion = Flight:new({
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

	onFlightModeChanged = function(self, locomotion, mode)
		if mode == "landed" then
			self:playVisual("landed")
		else
			self:playVisual("flying")
		end
	end,

	onFlightFacingChanged = function(self, locomotion, facing_right)
		self:setVisualFlip(facing_right, false)
	end,

	afterSpawn = function(self)
		self:playVisual("flying")
		if self.layer and self.layer.refreshEntityBuckets then
			self.layer:refreshEntityBuckets(self)
		end
		self.flight_locomotion:afterSpawn()
	end,

	selectAction = function(self)
		local threat = self:getPerceivedTargetForAction(Ecology.Actions.Flee)
		if threat and self:shouldFleeTarget(threat, self.flee_radius) then
			return Ecology.Actions.Flee, { target = threat }
		end
		if self:isState("landing") or self:isState("landed") then
			return Ecology.Actions.Perch, { target = self.flight_locomotion.landing_target }
		end
		return Ecology.Actions.Wander, { target = self.flight_locomotion.explore_target }
	end,

	update_agent = function(self)
		self.flight_locomotion:updateAutonomous()
	end,

	update = function(self)
		if Creature.update(self) == false then
			return
		end
		if Item.update(self) == false then
			return
		end
		WorldObject.update(self)
	end,
})

return Fly
