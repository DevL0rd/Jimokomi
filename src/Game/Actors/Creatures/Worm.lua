local Vector = include("src/Engine/Math/Vector.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Creature = include("src/Game/Mixins/Creature.lua")
local Item = include("src/Game/Mixins/Item.lua")
local GroundPatrol = include("src/Game/Actors/Locomotion/GroundPatrol.lua")
local Controller = include("src/Game/Actors/Creatures/Glider/Controller.lua")

local Worm = WorldObject:new({
	_type = "Worm",
	shape = {
		kind = "circle",
		r = 3,
	},
	edible = true,
	item_id = "worm",
	display_name = "Worm",
	can_pickup = true,
	can_use = true,
	can_drop = true,
	pickup_on_touch = true,
	pickup_radius_padding = 2,
	use_action_name = "eat",
	heal_amount = 1,
	max_stack_size = 1,
	spawn_item_path = "src/Game/Actors/Creatures/Worm.lua",
	replace_on_pickup = true,
	inventory_sprite = 0x1b,
	faction = Ecology.Factions.Wildlife,
	diet = Ecology.Diets.Herbivore,
	temperament = Ecology.Temperaments.Neutral,
	default_action = Ecology.Actions.Patrol,
	vision_range = 0,
	hearing_range = 0,
	action_plan_interval_ms = 250,
	perception_interval_ms = 500,
	sound_update_interval_ms = 500,
	sound_idle_radius = 10,
	sound_move_radius = 28,
	sound_land_radius = 18,
	sound_move_interval_ms = 900,
	sound_idle_interval_ms = 2600,
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
		Item.init(self)
		self.ignore_gravity = false
		self.ignore_friction = false
		self.ground_patrol_locomotion = GroundPatrol:new({
			owner = self,
			move_accel = self.move_accel,
			max_speed = self.max_speed,
		})
		self.controller = Controller:new({
			owner = self,
			directions = {
				left = -1,
				right = 1,
				up = -1,
				down = 1,
			},
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
		self.ground_patrol_locomotion:reverseDirection()
	end,
	onGroundPatrolDirectionChanged = function(self, locomotion, direction)
		self:setVisualFlip(direction > 0, false)
	end,
	afterSpawn = function(self)
		self:playVisual("crawl")
		if self.layer and self.layer.refreshEntityBuckets then
			self.layer:refreshEntityBuckets(self)
		end
		self:setState("patrol")
		self.ground_patrol_locomotion:afterSpawn()
	end,
	selectAction = function(self)
		local perceived = self:getPerceivedTargetForAction(Ecology.Actions.Watch)
		if perceived then
			return Ecology.Actions.Watch, { target = perceived }
		end
		return Ecology.Actions.Patrol, {}
	end,
	update_agent = function(self)
		if self:isPlayerControlled() and self.controller then
			local input = self.controller:getPlayerInputState()
			self.ground_patrol_locomotion:updateControlled(input)
			return
		end
		self.ground_patrol_locomotion:updateAutonomous()
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
	on_collision = function(self, ent, vector)
		if vector.x ~= 0 then
			self:reverseDirection()
		end
		Creature.on_collision(self, ent, vector)
	end
})

return Worm
