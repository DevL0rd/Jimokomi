local Vector = include("src/Engine/Math/Vector.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Creature = include("src/Game/Mixins/Creature.lua")
local Storage = include("src/Game/Mixins/Storage.lua")
local RaySensor = include("src/Engine/Sensors/RaySensor.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local EmitterNode = include("src/Engine/Nodes/EmitterNode.lua")
local Run = include("src/Game/Actors/Locomotion/Run.lua")
local Glide = include("src/Game/Actors/Locomotion/Glide.lua")
local Climb = include("src/Game/Actors/Locomotion/Climb.lua")
local Autonomy = include("src/Game/Actors/Creatures/Glider/Autonomy.lua")
local Controller = include("src/Game/Actors/Creatures/Glider/Controller.lua")
local Locomotion = include("src/Game/Actors/Creatures/Glider/Locomotion.lua")
local Visuals = include("src/Game/Actors/Creatures/Glider/Visuals.lua")
local Agent = include("src/Game/Mixins/Agent.lua")

local DIRECTIONS = {
	up = 0,
	down = 1,
	left = 2,
	right = 3,
}

local STATES = {
	Running = 1,
	Gliding = 2,
	Climbing = 3,
}

local CONTROL_MODES = {
	Player = Agent.ControlModes.Player,
	Autonomous = Agent.ControlModes.Autonomous,
}

local function is_visible_in_layer(self)
	local layer = self.layer
	local camera = layer and layer.camera or nil
	if not layer or not camera or not self.pos then
		return true
	end
	local radius = self.getRadius and self:getRadius() or 0
	return camera:isRectVisible(self.pos.x - radius, self.pos.y - radius, radius * 2, radius * 2)
end

local Glider = WorldObject:new({
	_type = "Glider",
	shape = {
		kind = "circle",
		r = 8,
	},
	fixed_rotation = true,
	resolve_entity_collisions = true,
	debug_collision_interest = false,
	faction = Ecology.Factions.Player,
	diet = Ecology.Diets.Omnivore,
	temperament = Ecology.Temperaments.Defensive,
	default_action = Ecology.Actions.Wander,
	display_name = "jiji",
	direction = DIRECTIONS.left,
	control_mode = CONTROL_MODES.Autonomous,
	player_accel = 180,
	climb_speed = 120,
	jump_speed = 120,
	inventory_slot_count = 3,
	inventory_slot_max_stack_size = 1,
	equipment_slots = { "head", "mouth", "back", "feet", "hands" },
	max_health = 6,
	health = 6,
	vision_range = 144,
	hearing_range = 160,
	prey_seek_radius = 120,
	prey_scan_interval = 1000,
	action_plan_interval_ms = 250,
	perception_interval_ms = 500,
	sound_update_interval_ms = 500,
	sound_move_interval_ms = 800,
	sound_idle_interval_ms = 2400,
	ground_probe_rate = 90,
	inactive_ground_probe_rate = 180,
	ground_probe_max_distance = 64,
	visual_definitions = {
		run_idle = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 64,
		},
		run_move = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 65,
			end_sprite = 69,
			speed = 16,
		},
		glide = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 70,
			end_sprite = 71,
			speed = 6,
		},
		climb_vertical_idle = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 72,
		},
		climb_vertical_move = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 73,
			end_sprite = 74,
			speed = 12,
		},
		climb_horizontal_idle = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 80,
		},
		climb_horizontal_move = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 81,
			end_sprite = 82,
			speed = 12,
		},
	},
	slot_defs = {
		visual = { x = 0, y = 0, destroy_policy = "destroy_children", max_children = 1 },
		ground_probe = { x = 0, y = 8, destroy_policy = "destroy_children", max_children = 1 },
		feet = { x = 0, y = 6, destroy_policy = "destroy_children", max_children = 1 },
	},

	exportState = function(self)
		local storage_state = self.getStorageState and self:getStorageState() or {}
		return {
			state = self.state,
			direction = self.direction,
			did_ground_pound = self.did_ground_pound,
			control_mode = self.control_mode,
			health = self.health,
			inventory = storage_state.inventory,
			equipment = storage_state.equipment,
		}
	end,

	importState = function(self, state)
		if not state then
			return
		end
		self.state = state.state or self.state
		self.direction = state.direction or self.direction
		self.control_mode = state.control_mode or self.control_mode
		self.health = mid(0, state.health or self.health or self.max_health or 1, self.max_health or 1)
		self.did_ground_pound = state.did_ground_pound ~= false
		if self.applyStorageState then
			self:applyStorageState(state)
		end
	end,

	isAutonomous = function(self)
		return self.control_mode == CONTROL_MODES.Autonomous
	end,

	isPlayerControlled = function(self)
		return self.control_mode == CONTROL_MODES.Player
	end,

	setControlMode = function(self, control_mode)
		self.control_mode = control_mode or CONTROL_MODES.Player
		if self.down_ray then
			if self:isPlayerControlled() then
				self.down_ray.rate = self.ground_probe_rate or 90
			else
				self.down_ray.rate = self.inactive_ground_probe_rate or 180
			end
		end
	end,

	heal = function(self, amount)
		self.health = mid(0, (self.health or self.max_health or 1) + (amount or 0), self.max_health or 1)
	end,

	isValidPrey = function(self, candidate)
		return candidate ~= nil and
			candidate ~= self and
			not candidate._delete and
			candidate.edible == true and
			candidate.can_pickup ~= true and
			candidate.layer == self.layer and
			candidate.pos ~= nil and
			candidate.is_spawned ~= false
	end,

	findClosestPrey = function(self, radius)
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		local scope = profiler and profiler:start("game.player.find_closest_prey") or nil
		local perceived = self:getPerceivedTarget()
		if self:isValidPrey(perceived) then
			local dx = perceived.pos.x - self.pos.x
			local dy = perceived.pos.y - self.pos.y
			local dist2 = dx * dx + dy * dy
			local max_dist2 = radius and radius * radius or nil
			if max_dist2 == nil or dist2 <= max_dist2 then
				if profiler then profiler:stop(scope) end
				return perceived
			end
		end

		local scan_interval = self.prey_scan_interval or 300
		if self.prey_scan_timer and self.prey_scan_timer:elapsed() < scan_interval then
			if self.cached_prey and not self.cached_prey._delete then
				if self:isValidPrey(self.cached_prey) then
					local dx = self.cached_prey.pos.x - self.pos.x
					local dy = self.cached_prey.pos.y - self.pos.y
					local dist2 = dx * dx + dy * dy
					local max_dist2 = radius and radius * radius or nil
					if max_dist2 == nil or dist2 <= max_dist2 then
						if profiler then
							profiler:addCounter("game.player.prey_cache_hits", 1)
							profiler:stop(scope)
						end
						return self.cached_prey
					end
				end
			end
			if profiler then profiler:stop(scope) end
			return nil
		end

		if self.prey_scan_timer and self.cached_prey and not self.cached_prey._delete then
			if self:isValidPrey(self.cached_prey) then
				local dx = self.cached_prey.pos.x - self.pos.x
				local dy = self.cached_prey.pos.y - self.pos.y
				local dist2 = dx * dx + dy * dy
				local max_dist2 = radius and radius * radius or nil
				if max_dist2 == nil or dist2 <= max_dist2 then
					if profiler then profiler:addCounter("game.player.prey_cache_hits", 1); profiler:stop(scope) end
					return self.cached_prey
				end
			end
		end

		local world = self:getWorld()
		if not self.layer or not world then
			if profiler then profiler:stop(scope) end
			return nil
		end

		local closest = nil
		local closest_dist2 = nil
		local max_dist2 = radius and radius * radius or nil
		local candidates = world:queryEntitiesInRadius(self.pos, radius or self.prey_seek_radius or 0, function(candidate)
			return self:isValidPrey(candidate)
		end)

		for i = 1, #candidates do
			local candidate = candidates[i]
			if self:isValidPrey(candidate) then
				local dx = candidate.pos.x - self.pos.x
				local dy = candidate.pos.y - self.pos.y
				local dist2 = dx * dx + dy * dy
				if (max_dist2 == nil or dist2 <= max_dist2) and (closest_dist2 == nil or dist2 < closest_dist2) then
					closest = candidate
					closest_dist2 = dist2
				end
			end
		end

		self.cached_prey = closest
		if self.prey_scan_timer then
			self.prey_scan_timer:reset()
		end
		if profiler then profiler:stop(scope) end
		return closest
	end,

	selectAction = function(self)
		if self:isPlayerControlled() then
			return Ecology.Actions.Idle, {}
		end

		local prey = self:findClosestPrey(self.prey_seek_radius)
		if prey then
			return Ecology.Actions.Hunt, { target = prey }
		end

		return Ecology.Actions.Wander, {}
	end,

	update = function(self)
		if Creature.update(self) == false then
			return
		end

		local input = self.controller:getInputState()
		local is_moving = input.left or input.right or input.up or input.down

		self.locomotion:updateStateFromWorld(input)
		self.controller:updateDirection(input)

		if input.jump then
			self.locomotion:jump(input)
		end

		self.locomotion:applyMovement(input, is_moving)
		if is_moving ~= self.old_is_moving or self.state ~= self.old_state or self.direction ~= self.old_direction then
			self.visuals:updateVisualState(is_moving)
		end

		self.old_direction = self.direction
		self.old_state = self.state
		self.old_is_moving = is_moving

		WorldObject.update(self)
	end,

	needsUpdate = function(self)
		if self:isPlayerControlled() then
			return true
		end
		if is_visible_in_layer(self) then
			return true
		end
		if self.offscreen_update_timer == nil then
		self.offscreen_update_interval_ms = self.offscreen_update_interval_ms or 150
		self.touch_pickup_active_padding = self.touch_pickup_active_padding or 32
			self.offscreen_update_timer = Timer:new()
			self.offscreen_update_timer.start_time -= random_int(0, self.offscreen_update_interval_ms) / 1000
		end
		return self.offscreen_update_timer:hasElapsed(self.offscreen_update_interval_ms or 150)
	end,
})

Glider.init = function(self)
	self.player_accel = self.player_accel or 180
	self.climb_speed = self.climb_speed or 120
	self.jump_speed = self.jump_speed or 120
	self.inventory_slot_count = self.inventory_slot_count or 3
	self.equipment_slots = self.equipment_slots or { "head", "mouth", "back", "feet", "hands" }
	self.control_mode = self.control_mode or CONTROL_MODES.Autonomous
	self.max_health = self.max_health or 6
	self.health = self.health or self.max_health
	self.prey_seek_radius = self.prey_seek_radius or 120
	self.prey_scan_interval = self.prey_scan_interval or 700
	self.ground_probe_rate = self.ground_probe_rate or 90
	self.inactive_ground_probe_rate = self.inactive_ground_probe_rate or 180
	self.ground_probe_max_distance = self.ground_probe_max_distance or 64
	self.offscreen_update_interval_ms = self.offscreen_update_interval_ms or 150
	WorldObject.init(self)
	Creature.init(self)
	Storage.init(self)

	self.glide_mode = false
	self.state = STATES.Running
	self.old_state = self.state
	self.old_direction = self.direction
	self.old_is_moving = false
	self.did_ground_pound = true
	self.jump_timer = Timer:new()
	self.prey_scan_timer = Timer:new()
	self.offscreen_update_timer = Timer:new()
	self.offscreen_update_timer.start_time -= random_int(0, self.offscreen_update_interval_ms) / 1000
	self.cached_prey = nil

	self.run_locomotion = Run:new({
		owner = self,
		horizontal_accel = self.player_accel,
	})
	self.glide_locomotion = Glide:new({
		owner = self,
		horizontal_accel = self.player_accel,
		lift = 2,
	})
	self.climb_locomotion = Climb:new({
		owner = self,
		speed = self.climb_speed,
	})
	self.autonomy = Autonomy:new({
		owner = self,
		prey_seek_radius = self.prey_seek_radius,
	})

	self.controller = Controller:new({
		owner = self,
		directions = DIRECTIONS,
	})
	self.locomotion = Locomotion:new({
		owner = self,
		states = STATES,
		directions = DIRECTIONS,
	})
	self.visuals = Visuals:new({
		owner = self,
		states = STATES,
		directions = DIRECTIONS,
	})
	self:playVisual("run_idle", { flip_x = false })

	self.down_ray = RaySensor:new({
		parent = self,
		parent_slot = "ground_probe",
		pos = Vector:new(),
		vec = Vector:new({ x = 0, y = 1 }),
		rate = self.ground_probe_rate,
	})
	self:setControlMode(self.control_mode)

	self.runEmitter = EmitterNode:new({
		parent = self,
		parent_slot = "feet",
		rate = 90,
		rate_variation = 20,
		particle_lifetime = 220,
		particle_lifetime_variation = 40,
		pos = Vector:new(),
		shape = {
			kind = "rect",
			w = self:getRadius(),
			h = 2,
		},
		particle_accel = Vector:new({ x = 0, y = -250 }),
	})
end

return Glider
