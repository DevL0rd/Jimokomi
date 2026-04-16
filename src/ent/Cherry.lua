local Circle = include("src/primitives/Circle.lua")
local Sprite = include("src/primitives/Sprite.lua")
local Spawner = include("src/utils/Spawner.lua")

local Cherry = Circle:new({
	_type = "Cherry",
	r = 5,
	ignore_physics = true,
	ignore_gravity = true,
	ignore_friction = true,
	ignore_collisions = true,
	is_spawned = false,
	spawn_tile_ids = { 32, 44 },
	offscreen_spawn_padding = 24,
	max_spawn_attempts = 24,
	eat_radius_padding = 2,
	init = function(self)
		Circle.init(self)
		self.graphics = Sprite:new({
			parent = self,
			w = 10,
			h = 10,
			sprite = 47,
		})
		self:trySpawn(false)
	end,
	getPlayer = function(self)
		if not self.layer or not self.layer.camera then
			return nil
		end
		return self.layer.camera.target
	end,
	setSpawnPosition = function(self, pos)
		self.pos.x = pos.x
		self.pos.y = pos.y
	end,
	respawn = function(self, require_offscreen)
		local spawn_pos = Spawner:getRandomTilePosition(
			self.layer,
			self.spawn_tile_ids,
			0,
			0,
			require_offscreen,
			self.r,
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

		Circle.update(self)
	end,
	draw = function(self)
		Circle.draw(self)
	end
})

return Cherry
