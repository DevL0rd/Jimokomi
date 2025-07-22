--[[pod_format="raw",created="2025-07-22 08:49:40",modified="2025-07-22 08:49:40",revision=0]]
local Vector = include("src/classes/Vector.lua")
local Circle = include("src/primitives/Circle.lua")
local Sprite = include("src/primitives/Sprite.lua")
local Ray = include("src/primitives/Ray.lua")
local ParticleEmitter = include("src/primitives/ParticleEmitter.lua")
local Rectangle = include("src/primitives/Rectangle.lua")

player_speed = 5
directions = {
	left = false,
	right = true
}
States = {
	Idle = 1,
	Moving = 2,
	Gliding = 3
}

local JIJI = Circle:new({
	_type = "JIJI",
	direction = directions.left,
	init = function(self)
		Circle.init(self)
		self.graphics = Sprite:new({
			parent = self,
			sprite = 0
		})
		self.old_direction = directions.left
		self.old_is_moving = false
		self.old_is_on_ground = false
		self.old_state = States.Idle
		self.down_ray = Ray:new({
			parent = self,
			pos = Vector:new({ x = 0, y = self.r }),
			dir = Vector:new({ x = 0, y = 1 })
		})
		self.state = States.Idle
		self.runEmitter = ParticleEmitter:new({
			parent = self,
			rate = 10,
			rate_variation = 50,
			particle_lifetime = 500,
			particle_lifetime_variation = 100,
			pos = Vector:new({ x = 0, y = self.r - 2 }),
			w = self.r,
			h = 2,
		})
		self.did_ground_pound = false
	end,
	distToGround = function(self)
		local obj, dist = self.down_ray:cast()
		return dist
	end,
	update = function(self)
		local ground_dist = self:distToGround()
		local is_on_ground = ground_dist <= 0.001
		local is_close_to_ground = ground_dist <= 2
		if btn(0) then
			self.direction = directions.left
		elseif btn(1) then
			self.direction = directions.right
		end
		if is_close_to_ground then
			if btn(0) then
				self.vel.x += -player_speed
				self.state = States.Moving
			elseif btn(1) then
				self.vel.x += player_speed
				self.state = States.Moving
			else
				self.state = States.Idle
			end
		else
			self.did_ground_pound = false
			self.state = States.Gliding
			self.vel.y -= 1
			local glideSpeed = self.vel.y > 0 and (self.vel.y * 0.1) or 0.6
			if self.direction == directions.left then
				self.vel.x -= glideSpeed
			else
				self.vel.x += glideSpeed
			end
		end
		local state_has_changed = self.direction ~= self.old_direction or self.state ~= self.old_state
		self.old_direction = self.direction
		self.old_state = self.state
		if state_has_changed then
			if self.state == States.Moving then
				self.graphics:destroy()
				self.graphics = Sprite:new({
					parent = self,
					sprite = 1,
					end_sprite = 5,
					flip_x = self.direction
				})
			elseif self.state == States.Gliding then
				self.graphics:destroy()
				self.graphics = Sprite:new({
					parent = self,
					sprite = 6,
					end_sprite = 7,
					speed = 6,
					flip_x = self.direction
				})
			else
				self.graphics:destroy()
				self.graphics = Sprite:new({
					parent = self,
					sprite = 0,
					flip_x = self.direction
				})
			end
		end

		if is_on_ground then
			if btn(2) then
				self.vel.y -= 60
			elseif self.state == States.Moving then
				self.vel.y -= 8
			end
		end
		if is_on_ground ~= self.old_is_on_ground then
			if is_on_ground and not self.did_ground_pound then
				self.world.camera:startShaking(5, 500)
				self.did_ground_pound = true
			end
		end
		self.old_is_on_ground = is_on_ground
		if self.state == States.Moving then
			self.runEmitter:on()
		else
			self.runEmitter:off()
		end
		Rectangle.update(self)
	end,
	draw = function(self)
		Rectangle.draw(self)
	end
})

return JIJI
