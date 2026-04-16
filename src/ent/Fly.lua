local Vector = include("src/classes/Vector.lua")
local Circle = include("src/primitives/Circle.lua")
local Sprite = include("src/primitives/Sprite.lua")
local Timer = include("src/classes/Timer.lua")

local Fly = Circle:new({
	_type = "Fly",
	r = 2,
	ignore_physics = true,
	ignore_gravity = true,
	ignore_friction = true,
	flutter_time = 0,
	wander_interval = 0,
	wander_velocity = nil,
	anchor = nil,
	is_spawned = false,
	spawn_margin = 16,
	eat_radius_padding = 2,
	offscreen_spawn_padding = 24,
	max_spawn_attempts = 24,
	init = function(self)
		Circle.init(self)
		self.wander_timer = Timer:new()
		self.anchor = Vector:new({ x = self.pos.x, y = self.pos.y })
		self.wander_velocity = Vector:new()
		self.graphics = Sprite:new({
			parent = self,
			w = 6,
			h = 3,
			sprite = 0x19,
			end_sprite = 0x1a,
			speed = 14,
		})
		self:trySpawn(false)
	end,
	getMapBounds = function(self)
		if not self.layer then
			return nil
		end
		return {
			left = self.spawn_margin,
			top = self.spawn_margin,
			right = self.layer.w - self.spawn_margin,
			bottom = self.layer.h - self.spawn_margin,
		}
	end,
	getPlayer = function(self)
		if not self.layer or not self.layer.camera then
			return nil
		end
		return self.layer.camera.target
	end,
	getRandomMapPosition = function(self)
		local bounds = self:getMapBounds()
		if not bounds then
			return Vector:new({ x = self.pos.x, y = self.pos.y })
		end
		return Vector:new({
			x = random_int(bounds.left, bounds.right),
			y = random_int(bounds.top, bounds.bottom)
		})
	end,
	isOnScreen = function(self, pos, padding)
		padding = padding or 0
		if not self.layer or not self.layer.camera then
			return false
		end
		return self.layer.camera:isRectVisible(
			pos.x - self.r - padding,
			pos.y - self.r - padding,
			(self.r + padding) * 2,
			(self.r + padding) * 2
		)
	end,
	getRandomOffscreenPosition = function(self)
		for i = 1, self.max_spawn_attempts do
			local candidate = self:getRandomMapPosition()
			if not self:isOnScreen(candidate, self.offscreen_spawn_padding) then
				return candidate
			end
		end
		return self:getRandomMapPosition()
	end,
	resetWander = function(self)
		self.wander_timer:reset()
		self.wander_interval = random_int(250, 700)
		self.wander_velocity.x = random_float(-14, 14)
		self.wander_velocity.y = random_float(-10, 10)
		if self.graphics then
			self.graphics.flip_x = self.wander_velocity.x > 0
		end
	end,
	setAnchorPosition = function(self, pos)
		self.anchor.x = pos.x
		self.anchor.y = pos.y
		self.pos.x = pos.x
		self.pos.y = pos.y
	end,
	respawn = function(self, require_offscreen)
		local spawn_pos
		if require_offscreen then
			spawn_pos = self:getRandomOffscreenPosition()
		else
			spawn_pos = self:getRandomMapPosition()
		end
		self:setAnchorPosition(spawn_pos)
		self.flutter_time = random_float(0, 6.28318)
		self:resetWander()
		self.is_spawned = true
	end,
	trySpawn = function(self, require_offscreen)
		if not self.layer then
			return false
		end
		self:respawn(require_offscreen)
		return true
	end,
	isEatenByPlayer = function(self)
		local player = self:getPlayer()
		if not player then
			return false
		end
		local dx = player.pos.x - self.pos.x
		local dy = player.pos.y - self.pos.y
		local eat_radius = player.r + self.r + self.eat_radius_padding
		return dx * dx + dy * dy <= eat_radius * eat_radius
	end,
	update = function(self)
		if not self.is_spawned then
			if not self:trySpawn(false) then
				Circle.update(self)
				return
			end
		end

		if self:isEatenByPlayer() then
			self:trySpawn(true)
		end

		if self.wander_timer:elapsed() >= self.wander_interval then
			self:resetWander()
		end

		self.flutter_time += _dt * 7

		local home_x = (self.anchor.x - self.pos.x) * 0.8
		local home_y = (self.anchor.y - self.pos.y) * 0.8
		local bob_y = sin(self.flutter_time) * 8

		self.pos.x += (self.wander_velocity.x + home_x) * _dt
		self.pos.y += (self.wander_velocity.y + home_y + bob_y) * _dt

		Circle.update(self)
	end,
	draw = function(self)
		Circle.draw(self)
	end
})

return Fly
