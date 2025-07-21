player_speed = 180
directions = {
	left = false,
	right = true
}
States = {
	Idle = 1,
	Moving = 2,
	Gliding = 3
}

Jiji = Circle:new({
	_type = "jiji",
	direction = directions.left,
	graphics = nil,
	init = function(self)
		Circle.init(self)
		self.graphics = Graphic:new({
			world = self.world,
			sprite = 0
		})
		self.old_direction = directions.left
		self.old_is_moving = false
		self.old_is_on_ground = false
		self.old_state = States.Idle
		self.down_ray = Ray:new({
			world = self.world
		})
		self.state = States.Idle
		self.runEmitter = ParticleEmitter:new({
			world = self.world,
			rate = 10,
			rate_variation = 50,
			particle_lifetime = 500,
			particle_lifetime_variation = 100,
			w = self.r,
			h = self.r,
		})
	end,
	distToGround = function(self)
		local obj, dist = self.down_ray:cast()
		return dist
	end,
	update = function(self)
		if self.world and not self.down_ray.world then
			self.down_ray.world = self.world
		end

		self.down_ray.pos.x = self.pos.x + self.r
		self.down_ray.pos.y = self.pos.y + self.r
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
				self.vel.x = -player_speed
				self.state = States.Moving
			elseif btn(1) then
				self.vel.x = player_speed
				self.state = States.Moving
			else
				self.state = States.Idle
			end
		else
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
				self.graphics = Graphic:new({
					world = self.world,
					sprite = 1,
					end_sprite = 5,
					flip_x = self.direction
				})
			elseif self.state == States.Gliding then
				self.graphics = Graphic:new({
					world = self.world,
					sprite = 6,
					end_sprite = 7,
					speed = 6,
					flip_x = self.direction
				})
			else
				self.graphics = Graphic:new({
					world = self.world,
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
		self.runEmitter.pos.x = self.pos.x
		self.runEmitter.pos.y = self.pos.y + self.r
		if self.state == States.Moving then
			self.runEmitter:on()
		else
			self.runEmitter:off()
		end
		self.runEmitter:update()
		Rectangle.update(self)
	end,
	draw = function(self)
		self.down_ray:draw()
		self.runEmitter:draw()
		Rectangle.draw(self)
	end
})
