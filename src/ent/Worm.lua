local Circle = include("src/primitives/Circle.lua")
local Sprite = include("src/primitives/Sprite.lua")
local Vector = include("src/classes/Vector.lua")
local Spawner = include("src/utils/Spawner.lua")

local Worm = Circle:new({
	_type = "Worm",
	r = 3,
	sprite_offset_y = 6,
	move_accel = 70,
	max_speed = 18,
	direction = -1,
	ignore_physics = true,
	is_spawned = false,
	spawn_margin = 16,
	eat_radius_padding = 2,
	offscreen_spawn_padding = 24,
	max_spawn_attempts = 24,
	init = function(self)
		Circle.init(self)
		self.ignore_gravity = false
		self.ignore_friction = false
		self.graphics = Sprite:new({
			parent = self,
			pos = Vector:new({ y = self.sprite_offset_y }),
			w = 6,
			h = 1,
			sprite = 0x1b,
			end_sprite = 0x1c,
			speed = 6,
		})
		self:trySpawn(false)
	end,
	getPlayer = function(self)
		if not self.layer or not self.layer.camera then
			return nil
		end
		return self.layer.camera.target
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
	isSolidAt = function(self, x, y)
		if not self.layer or not self.layer.collision then
			return false
		end
		local tile_id = self.layer.collision:getTileIDAt(x, y, self.layer.map_id)
		return self.layer.collision:isSolidTile(tile_id)
	end,
	isOnGround = function(self)
		return self:isSolidAt(self.pos.x, self.pos.y + self.r + 1)
	end,
	isBlockedAhead = function(self)
		local probe_x = self.pos.x + self.direction * (self.r + 2)
		return self:isSolidAt(probe_x, self.pos.y)
	end,
	hasGroundAhead = function(self)
		local probe_x = self.pos.x + self.direction * (self.r + 4)
		local probe_y = self.pos.y + self.r + 2
		return self:isSolidAt(probe_x, probe_y)
	end,
	reverseDirection = function(self)
		self.direction = -self.direction
		if self.graphics then
			self.graphics.flip_x = self.direction > 0
		end
	end,
	setSpawnPosition = function(self, pos)
		self.pos.x = pos.x
		self.pos.y = pos.y
		self.vel.x = 0
		self.vel.y = 0
		self.direction = random_int(0, 1) == 0 and -1 or 1
		if self.graphics then
			self.graphics.flip_x = self.direction > 0
		end
	end,
	respawn = function(self, require_offscreen)
		local spawn_pos = Spawner:getRandomSurfacePosition(
			self.layer,
			self.r,
			self.spawn_margin,
			require_offscreen,
			self.offscreen_spawn_padding,
			self.max_spawn_attempts
		)
		if not spawn_pos then
			return false
		end
		self:setSpawnPosition(spawn_pos)
		self.is_spawned = true
		return true
	end,
	trySpawn = function(self, require_offscreen)
		if not self.layer then
			return false
		end
		return self:respawn(require_offscreen)
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

		if self:isOnGround() then
			if self:isBlockedAhead() or not self:hasGroundAhead() then
				self:reverseDirection()
			end

			self.vel.x += self.direction * self.move_accel * _dt
			self.vel.x = mid(-self.max_speed, self.vel.x, self.max_speed)
		end

		Circle.update(self)
	end,
	on_collision = function(self, ent, vector)
		if vector.x ~= 0 then
			self:reverseDirection()
		end
		Circle.on_collision(self, ent, vector)
	end,
	draw = function(self)
		Circle.draw(self)
	end
})

return Worm
