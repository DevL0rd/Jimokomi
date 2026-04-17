local Vector = include("src/classes/Vector.lua")
local Ecology = include("src/classes/Ecology.lua")
local WorldObject = include("src/primitives/WorldObject.lua")
local VisualOwner = include("src/primitives/VisualOwner.lua")
local Ray = include("src/primitives/Ray.lua")
local Timer = include("src/classes/Timer.lua")
local ParticleEmitter = include("src/primitives/ParticleEmitter.lua")
local RunBehavior = include("src/classes/behaviors/RunBehavior.lua")
local GlideBehavior = include("src/classes/behaviors/GlideBehavior.lua")
local ClimbBehavior = include("src/classes/behaviors/ClimbBehavior.lua")
local PlayerController = include("src/classes/player/PlayerController.lua")
local PlayerLoadout = include("src/classes/player/PlayerLoadout.lua")
local PlayerLocomotion = include("src/classes/player/PlayerLocomotion.lua")
local PlayerVisuals = include("src/classes/player/PlayerVisuals.lua")

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

local Jiji = WorldObject:new({
	_type = "Jiji",
	shape = {
		kind = "circle",
		r = 8,
	},
	faction = Ecology.Factions.Player,
	diet = Ecology.Diets.Omnivore,
	temperament = Ecology.Temperaments.Defensive,
	visual_key = "Jiji",
	direction = DIRECTIONS.left,
	slot_defs = {
		visual = { x = 0, y = 0, destroy_policy = "destroy_children", max_children = 1 },
		ground_probe = { x = 0, y = 8, destroy_policy = "destroy_children", max_children = 1 },
		feet = { x = 0, y = 6, destroy_policy = "destroy_children", max_children = 1 },
	},

	exportState = function(self)
		local loadout_state = self.loadout and self.loadout:exportState() or {}
		return {
			state = self.state,
			direction = self.direction,
			did_ground_pound = self.did_ground_pound,
			inventory = loadout_state.inventory,
			equipment = loadout_state.equipment,
		}
	end,

	importState = function(self, state)
		if not state then
			return
		end
		self.state = state.state or self.state
		self.direction = state.direction or self.direction
		self.did_ground_pound = state.did_ground_pound ~= false
		if self.loadout then
			self.loadout:importState(state)
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

Jiji.init = function(self)
	local assets = self.layer and self.layer.engine and self.layer.engine.assets or nil
	if assets then
		assets:applyEntityConfig(self)
	end
	self.player_accel = self.player_accel or 180
	self.climb_speed = self.climb_speed or 120
	self.jump_speed = self.jump_speed or 120
	self.inventory_size = self.inventory_size or 16
	self.equipment_slots = self.equipment_slots or { "head", "mouth", "back", "feet", "hands" }
	WorldObject.init(self)
	VisualOwner.init(self)

	self.glide_mode = false
	self.state = STATES.Running
	self.old_state = self.state
	self.old_direction = self.direction
	self.old_is_moving = false
	self.did_ground_pound = true
	self.jump_timer = Timer:new()

	self.run_behavior = RunBehavior:new({
		owner = self,
		horizontal_accel = self.player_accel,
	})
	self.glide_behavior = GlideBehavior:new({
		owner = self,
		horizontal_accel = self.player_accel,
		lift = 2,
	})
	self.climb_behavior = ClimbBehavior:new({
		owner = self,
		speed = self.climb_speed,
	})

	self.controller = PlayerController:new({
		owner = self,
		directions = DIRECTIONS,
	})
	self.loadout = PlayerLoadout:new({
		owner = self,
	})
	self.locomotion = PlayerLocomotion:new({
		owner = self,
		states = STATES,
		directions = DIRECTIONS,
	})
	self.visuals = PlayerVisuals:new({
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

return Jiji
