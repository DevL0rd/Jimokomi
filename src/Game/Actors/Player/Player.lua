local Vector = include("src/Engine/Math/Vector.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local VisualOwner = include("src/Engine/Mixins/VisualOwner.lua")
local Storage = include("src/Game/Mixins/Storage.lua")
local Ray = include("src/Engine/Objects/Ray.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local ParticleEmitter = include("src/Engine/Objects/ParticleEmitter.lua")
local Run = include("src/Game/Actors/Locomotion/Run.lua")
local Glide = include("src/Game/Actors/Locomotion/Glide.lua")
local Climb = include("src/Game/Actors/Locomotion/Climb.lua")
local Controller = include("src/Game/Actors/Player/Controller.lua")
local Locomotion = include("src/Game/Actors/Player/Locomotion.lua")
local Visuals = include("src/Game/Actors/Player/Visuals.lua")

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

local Player = WorldObject:new({
	_type = "Player",
	shape = {
		kind = "circle",
		r = 8,
	},
	faction = Ecology.Factions.Player,
	diet = Ecology.Diets.Omnivore,
	temperament = Ecology.Temperaments.Defensive,
	direction = DIRECTIONS.left,
	player_accel = 180,
	climb_speed = 120,
	jump_speed = 120,
	inventory_slot_count = 16,
	inventory_slot_max_stack_size = nil,
	equipment_slots = { "head", "mouth", "back", "feet", "hands" },
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
		self.did_ground_pound = state.did_ground_pound ~= false
		if self.applyStorageState then
			self:applyStorageState(state)
		end
	end,

	update = function(self)
		local input = self.controller:getInputState()
		local is_moving = input.left or input.right or input.up or input.down

		self.locomotion:updateStateFromWorld(input)
		self.controller:updateDirection(input)

		if input.jump then
			self.locomotion:jump(input)
		end

		self.locomotion:applyMovement(input, is_moving)
		self.visuals:updateVisualState(is_moving)

		self.old_direction = self.direction
		self.old_state = self.state
		self.old_is_moving = is_moving

		WorldObject.update(self)
	end,
})

Player.init = function(self)
	self.player_accel = self.player_accel or 180
	self.climb_speed = self.climb_speed or 120
	self.jump_speed = self.jump_speed or 120
	self.inventory_slot_count = self.inventory_slot_count or 16
	self.equipment_slots = self.equipment_slots or { "head", "mouth", "back", "feet", "hands" }
	WorldObject.init(self)
	VisualOwner.init(self)
	Storage.init(self)

	self.glide_mode = false
	self.state = STATES.Running
	self.old_state = self.state
	self.old_direction = self.direction
	self.old_is_moving = false
	self.did_ground_pound = true
	self.jump_timer = Timer:new()

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

	self.down_ray = Ray:new({
		parent = self,
		parent_slot = "ground_probe",
		pos = Vector:new(),
		vec = Vector:new({ x = 0, y = 1 }),
	})

	self.runEmitter = ParticleEmitter:new({
		parent = self,
		parent_slot = "feet",
		rate = 10,
		rate_variation = 50,
		particle_lifetime = 500,
		particle_lifetime_variation = 100,
		pos = Vector:new(),
		shape = {
			kind = "rect",
			w = self:getRadius(),
			h = 2,
		},
		particle_accel = Vector:new({ x = 0, y = -250 }),
	})
end

return Player
