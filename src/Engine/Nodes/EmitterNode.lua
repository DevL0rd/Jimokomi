local Timer = include("src/Engine/Core/Timer.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local AttachmentNode = include("src/Engine/Nodes/AttachmentNode.lua")
local Particle = include("src/Engine/Objects/Particle.lua")

local EmitterNode = AttachmentNode:new({
	_type = "EmitterNode",
	node_kind = 2,
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
	state = true,
	Particle = Particle,

	init = function(self)
		AttachmentNode.init(self)
		self.spawn_timer = Timer:new()
		self.waiting_time = self.rate
	end,

	needsNodeUpdate = function(self)
		return self.state == true
	end,

	getHalfWidth = function(self)
		return ((self.shape and self.shape.w) or 0) * 0.5
	end,

	getHalfHeight = function(self)
		return ((self.shape and self.shape.h) or 0) * 0.5
	end,

	getRandomSpawnPosition = function(self)
		local half_width = self:getHalfWidth()
		local half_height = self:getHalfHeight()
		return Vector:new({
			x = self.pos.x + random_int(-half_width, half_width),
			y = self.pos.y + random_int(-half_height, half_height),
		})
	end,

	spawnParticle = function(self)
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		if profiler then
			profiler:addCounter("render.emitter_node.particles_spawned", 1)
		end
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
				y = self.vec.y + random_int(-self.vec_variation, self.vec_variation),
			}),
			shape = {
				kind = "circle",
				r = particle_size,
			},
			accel = self.particle_accel,
		})
	end,

	updateNode = function(self)
		local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
		if profiler and self.state then
			profiler:addCounter("render.emitter_node.active", 1)
		end
		if self.state and self.spawn_timer:hasElapsed(self.waiting_time) then
			self.waiting_time = self.rate + random_int(-self.rate_variation, self.rate_variation)
			if self.waiting_time < 1 then
				self.waiting_time = 1
			end
			self:spawnParticle()
		end
	end,

	toggle = function(self)
		self.state = not self.state
	end,

	on = function(self)
		self.state = true
	end,

	off = function(self)
		self.state = false
	end,
})

return EmitterNode
