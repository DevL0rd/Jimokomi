--[[pod_format="raw",created="2025-07-22 08:49:40",modified="2025-07-23 07:38:17",revision=4]]
local Vector = include("src/classes/Vector.lua")
local Circle = include("src/primitives/Circle.lua")
local Sprite = include("src/primitives/Sprite.lua")
local Ray = include("src/primitives/Ray.lua")
local Timer = include("src/classes/Timer.lua")
local ParticleEmitter = include("src/primitives/ParticleEmitter.lua")

player_accel = 160
jump_speed = 120
directions = {
	up = 0,
	down = 1,
	left = 2,
	right = 3,
}
States = {
	Running = 1,
	Gliding = 2,
	Climbing = 3,
}

local JIJI = Circle:new({
	_type = "JIJI",
	direction = directions.left,
	init = function(self)
		Circle.init(self)
		self.graphics = Sprite:new({
			parent = self,
			sprite = 64
		})
		self.glide_mode = false
		self.old_direction = directions.left
		self.old_is_running = false
		self.old_is_on_ground = false
		self.old_state = States.Running
		self.down_ray = Ray:new({
			parent = self,
			pos = Vector:new({ x = 0, y = self.r }),
			dir = Vector:new({ x = 0, y = 1 })
		})
		self.state = States.Running
		self.runEmitter = ParticleEmitter:new({
			parent = self,
			rate = 10,
			rate_variation = 50,
			particle_lifetime = 500,
			particle_lifetime_variation = 100,
			pos = Vector:new({ x = 0, y = self.r - 2 }),
			w = self.r,
			h = 2,
			particle_accel = Vector:new({ x = 0, y = -250 }),
		})
		self.did_ground_pound = true
		self.jump_timer = Timer:new()
		self.old_is_moving = false
	end,
	distToGround = function(self)
		local obj, dist = self.down_ray:cast()
		return dist
	end,
	isOnGround = function(self)
		local dist = self:distToGround()
		return dist <= 0.001
	end,
	is_close_to_ground = function(self)
		local dist = self:distToGround()
		return dist <= 2
	end,
	jump = function(self)
		if not self.jump_timer:hasElapsed(500) then
			return false
		end
		if self.state == States.Running then
			self.vel.y -= jump_speed

			local is_moving = btn(0) or btn(1) or btn(2) or btn(3)
			if is_moving then
				if self.direction == directions.left then
					self.vel.x -= jump_speed * 0.5
				elseif self.direction == directions.right then
					self.vel.x += jump_speed * 0.5
				end
			end
			return true
		elseif self.state == States.Climbing then
			self.state = States.Gliding -- Force gliding state so when climbing, it will glide
			if self.direction == directions.left then
				self.vel.y -= jump_speed
				self.vel.x -= jump_speed * 0.5
			elseif self.direction == directions.right then
				self.vel.y -= jump_speed
				self.vel.x += jump_speed * 0.5
			end
			if self.direction == directions.up then
				self.vel.y -= jump_speed
			elseif self.direction == directions.down then
				self.vel.y += jump_speed
			end
			return true
		elseif self.state == States.Gliding then
			local current_tile = self.layer.collision:getTileIDAt(self.pos.x, self.pos.y, 0)
			if current_tile ~= 0 and current_tile ~= 15 then
				self.state = States.Climbing
			end
			return true
		end
		return false
	end,
	update = function(self)
		local current_tile = self.layer.collision:getTileIDAt(self.pos.x, self.pos.y, 0)
		print_debug("Current tile: " .. current_tile)
		if current_tile ~= 0 and current_tile ~= 15 and (btn(2) or btn(3)) then
			self.state = States.Climbing
		end
		if self.state == States.Climbing and not (current_tile ~= 0 and current_tile ~= 15) then
			self.state = States.Gliding
			self.vel:drag(0.5) -- Apply some drag when leaving climbing state
		end
		if self.state ~= States.Climbing then
			if self:is_close_to_ground() then
				self.state = States.Running
			elseif self.state then
				self.state = States.Gliding
			end
		end


		if btn(2) then
			self.direction = directions.up
		elseif btn(0) then
			self.direction = directions.left
		elseif btn(1) then
			self.direction = directions.right
		elseif btn(3) then
			self.direction = directions.down
		end
		if btn(4) then
			self:jump()
		end

		local is_moving = btn(0) or btn(1) or btn(2) or btn(3)
		local state_has_changed = self.direction ~= self.old_direction or self.state ~= self.old_state or
			is_moving ~= self.old_is_moving

		self.old_direction = self.direction
		self.old_state = self.state
		self.old_is_moving = is_moving

		self.runEmitter:off()
		self.ignore_gravity = false
		if self.state == States.Climbing then
			self.ignore_gravity = true
			self.vel.y = 0
			self.vel.x = 0
			if btn(0) then
				self.vel.x = -player_accel
			elseif btn(1) then
				self.vel.x = player_accel
			end
			if btn(2) then
				self.vel.y = -player_accel
			elseif btn(3) then
				self.vel.y = player_accel
			end
		elseif self.state == States.Running then
			if is_moving then
				if self.direction == directions.left then
					self.vel.x -= player_accel * _dt
				elseif self.direction == directions.right then
					self.vel.x += player_accel * _dt
				end
				if self:isOnGround() then
					self.vel.y -= 8
				end
				self.runEmitter.vec.x = -self.vel.x * 0.2
				self.runEmitter.vec.y = -30
				self.runEmitter:on()
			end
		elseif self.state == States.Gliding then
			self.did_ground_pound = false
			self.vel.y -= 2
			if self.direction == directions.left then
				self.vel.x -= player_accel * _dt
			elseif self.direction == directions.right then
				self.vel.x += player_accel * _dt
			end
		end


		if state_has_changed then
			self.graphics:destroy()
			if self.state == States.Running then
				if not self.did_ground_pound then
					self.layer.camera:startShaking(5, 500)
					self.did_ground_pound = true
				end
				if is_moving then
					self.graphics = Sprite:new({
						parent = self,
						sprite = 65,
						end_sprite = 69,
						speed = 6,
						flip_x = self.direction == directions.right
					})
				else
					self.graphics = Sprite:new({
						parent = self,
						sprite = 64,
						flip_x = self.direction == directions.right
					})
				end
			elseif self.state == States.Gliding then
				self.graphics = Sprite:new({
					parent = self,
					sprite = 70,
					end_sprite = 71,
					speed = 6,
					flip_x = self.direction == directions.right
				})
			elseif self.state == States.Climbing then
				if is_moving then
					if self.direction == directions.up then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 73,
							end_sprite = 74,
							speed = 6
						})
					elseif self.direction == directions.down then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 73,
							end_sprite = 74,
							speed = 6,
							flip_y = true,
						})
					elseif self.direction == directions.left then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 81,
							end_sprite = 82,
							speed = 6
						})
					elseif self.direction == directions.right then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 81,
							end_sprite = 82,
							speed = 6,
							flip_x = true
						})
					end
				else
					if self.direction == directions.up then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 72,
						})
					elseif self.direction == directions.down then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 72,
							flip_y = true,
						})
					elseif self.direction == directions.left then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 80,
						})
					elseif self.direction == directions.right then
						self.graphics = Sprite:new({
							parent = self,
							sprite = 80,
							flip_x = true
						})
					end
				end
			end
		end
		Circle.update(self)
	end,
	draw = function(self)
		Circle.draw(self)
	end
})

return JIJI
