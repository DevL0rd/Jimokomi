--[[pod_format="raw",created="2025-07-22 08:49:40",modified="2025-07-22 08:49:40",revision=0]]
local Vector = include("src/classes/Vector.lua")
local Circle = include("src/primitives/Circle.lua")
local Sprite = include("src/primitives/Sprite.lua")
local ParticleEmitter = include("src/primitives/ParticleEmitter.lua")

local CoCo = Circle:new({
	_type = "CoCo",
	direction = directions.left,
	init = function(self)
		Circle.init(self)
		self.graphics = Sprite:new({
			parent = self,
			sprite = 8,
			end_sprite = 9,
			speed = 1,
		})
		self.sleepEmitter = ParticleEmitter:new({
			parent = self,
			rate = 2000,
			particle_lifetime = 500,
			particle_lifetime_variation = 100,
			pos = Vector:new({ x = 0, y = -5 }),
			w = self.r,
			h = 2,
		})
		self.did_ground_pound = false
	end,
	draw = function(self)
		Circle.draw(self)
	end
})

return CoCo
