local Timer = include("src/Engine/Core/Timer.lua")
local Particle = include("src/Engine/Objects/Particle.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local WorldObject = include("src/Engine/Objects/WorldObject.lua")

local ParticleEmitter = WorldObject:new({
	_type = "ParticleEmitter",
	shape = {
		kind = "rect",
		w = 16,
		h = 16,
	},
	rate = 100,
	rate_variation = 0,
	particle_lifetime = 500,
	particle_lifetime_variation = 0,
	particle_size = 3,
	particle_size_variation = 2,
	particle_accel = Vector:new({ x = 0, y = 0 }),
	vec = Vector:new({ x = 0, y = 0 }),
	vec_variation = 0,
	ignore_physics = true,
	ignore_collisions = true,
	snapshot_enabled = false,
	state = true,
	Particle = Particle,

	init = function(self)
		WorldObject.init(self)
		self.spawn_timer = Timer:new()
		self.waiting_time = self.rate
	end,

	getRandomSpawnPosition = function(self)
		local half_width = self:getHalfWidth()
		local half_height = self:getHalfHeight()
		return Vector:new({
			x = self.pos.x + random_int(-half_width, half_width),
			y = self.pos.y + random_int(-half_height, half_height)
		})
	end,

	spawnParticle = function(self)
		local particle_size = self.particle_size + random_int(-self.particle_size_variation, self.particle_size_variation)
		if particle_size < 1 then
			particle_size = 1
		end

		return self.Particle:new({
			layer = self.layer,
			lifetime = self.particle_lifetime +
				random_int(-self.particle_lifetime_variation, self.particle_lifetime_variation),
			pos = self:getRandomSpawnPosition(),
			vel = Vector:new({
				x = self.vec.x + random_int(-self.vec_variation, self.vec_variation),
				y = self.vec.y + random_int(-self.vec_variation, self.vec_variation)
			}),
			shape = {
				kind = "circle",
				r = particle_size,
			},
			accel = self.particle_accel,
		})
	end,

	update = function(self)
		if self.state and self.spawn_timer:hasElapsed(self.waiting_time) then
			self.waiting_time = self.rate + random_int(-self.rate_variation, self.rate_variation)
			if self.waiting_time < 1 then
				self.waiting_time = 1
			end
			self:spawnParticle()
		end
		WorldObject.update(self)
	end,
	needsUpdate = function(self)
		return self.state or self.lifetime ~= -1
	end,

	draw_debug = function(self)
		local top_left = self:getTopLeft()
		local width = self:getWidth()
		local height = self:getHeight()
		self.layer.gfx:rect(top_left.x, top_left.y, top_left.x + width - 1, top_left.y + height - 1, 16)
		local vec_end_x = self.pos.x + self.vec.x * 0.25
		local vec_end_y = self.pos.y + self.vec.y * 0.25
		self.layer.gfx:line(self.pos.x, self.pos.y, vec_end_x, vec_end_y, 16)
	end,

	toggle = function(self)
		self.state = not self.state
	end,

	on = function(self)
		self.state = true
	end,

	off = function(self)
		self.state = false
	end
})

return ParticleEmitter
