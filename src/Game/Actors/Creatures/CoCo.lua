--[[pod_format="raw",created="2025-07-22 08:49:40",modified="2025-07-22 08:49:40",revision=0]]
local Vector = include("src/Engine/Math/Vector.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Creature = include("src/Game/Mixins/Creature.lua")
local ParticleEmitter = include("src/Engine/Objects/ParticleEmitter.lua")
local zzz = include("src/Game/Effects/Particles/zzz.lua")

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
		self.sleepEmitter = ParticleEmitter:new({
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
	update = function(self)
		if Creature.update(self) == false then
			return
		end
		WorldObject.update(self)
	end,
})

return CoCo
