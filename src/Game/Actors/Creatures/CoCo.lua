--[[pod_format="raw",created="2025-07-22 08:49:40",modified="2025-07-22 08:49:40",revision=0]]
local Vector = include("src/Engine/Math/Vector.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Creature = include("src/Game/Mixins/Creature.lua")
local EmitterNode = include("src/Engine/Nodes/EmitterNode.lua")
local zzz = include("src/Game/Effects/Particles/zzz.lua")
local Controller = include("src/Game/Actors/Creatures/Glider/Controller.lua")

local CoCo = WorldObject:new({
	_type = "CoCo",
	shape = {
		kind = "circle",
		r = 8,
	},
	slot_defs = {
		sleep = { x = 0, y = -5, destroy_policy = "destroy_children", max_children = 1 },
	},
	faction = Ecology.Factions.Wildlife,
	diet = Ecology.Diets.Herbivore,
	temperament = Ecology.Temperaments.Passive,
	vision_range = 64,
	hearing_range = 80,
	sound_idle_radius = 22,
	sound_move_radius = 0,
	sound_land_radius = 0,
	control_speed = 48,
	visual_definitions = {
		sleep = {
			shape = { kind = "rect", w = 16, h = 16 },
			sprite = 120,
			end_sprite = 121,
			speed = 1,
		},
	},
	init = function(self)
		WorldObject.init(self)
		Creature.init(self)
		local radius = self:getRadius()
		self:playVisual("sleep")
		self.controller = Controller:new({
			owner = self,
			directions = {
				up = 0,
				down = 1,
				left = 2,
				right = 3,
			},
		})
		self.sleepEmitter = EmitterNode:new({
			parent = self,
			parent_slot = "sleep",
			rate = 2000,
			Particle = zzz,
			particle_lifetime = 500,
			particle_lifetime_variation = 100,
			pos = Vector:new(),
			vec = Vector:new({ x = 0, y = -50 }),
			particle_accel = Vector:new({ x = 0, y = -210 }),
			shape = {
				kind = "rect",
				w = radius,
				h = 2,
			},
		})
	end,
	update_agent = function(self)
		if not self:isPlayerControlled() or not self.controller then
			return
		end

		local input = self.controller:getPlayerInputState()
		local dx = 0
		local dy = 0
		if input.left then
			dx -= 1
		end
		if input.right then
			dx += 1
		end
		if input.up then
			dy -= 1
		end
		if input.down then
			dy += 1
		end
		if dx ~= 0 or dy ~= 0 then
			local dist = sqrt(dx * dx + dy * dy)
			dx /= dist
			dy /= dist
			self.pos.x += dx * self.control_speed * _dt
			self.pos.y += dy * self.control_speed * _dt
			self.controller:updateDirection(input)
		end
	end,
	update = function(self)
		if Creature.update(self) == false then
			return
		end
		WorldObject.update(self)
	end,
})

return CoCo
