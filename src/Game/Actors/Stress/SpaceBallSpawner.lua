local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local SpaceBall = include("src/Game/Actors/Stress/SpaceBall.lua")
local Vector = include("src/Engine/Math/Vector.lua")

local TAU = 6.28318

local SpaceBallSpawner = WorldObject:new({
	_type = "SpaceBallSpawner",
	inherit_layer_debug = false,
	ignore_physics = true,
	ignore_gravity = true,
	ignore_friction = true,
	ignore_collisions = true,
	spawn_interval_ms = 60,
	spawn_per_tick = 2,
	max_space_balls = 20,
	spawn_radius = 14,
	launch_speed = 110,
	upward_bias = 55,

	init = function(self)
		WorldObject.init(self)
		self.spawn_timer = Timer:new()
		self.spawn_count = 0
	end,

	getLiveBallCount = function(self)
		local layer = self.layer
		if not layer then
			return 0
		end
		return max(0, layer._stress_space_ball_count or 0)
	end,

	getSpawnCenter = function(self)
		local layer = self.layer
		return layer and layer.w and layer.w * 0.5 or 0, layer and layer.h and layer.h * 0.5 or 0
	end,

	spawnBall = function(self)
		local world = self:getWorld()
		local layer = self.layer
		if not world or not layer then
			return nil
		end
		local center_x, center_y = self:getSpawnCenter()
		local index = self.spawn_count or 0
		local angle = (index * 0.6180339 % 1) * TAU
		local ring = (index % 7) / 6
		local spawn_x = center_x + cos(angle) * self.spawn_radius * ring
		local spawn_y = center_y + sin(angle) * self.spawn_radius * ring
		local velocity_x = cos(angle) * self.launch_speed
		local velocity_y = sin(angle) * self.launch_speed - self.upward_bias
		self.spawn_count = index + 1
		return world:spawn(SpaceBall, {
			layer = layer,
			pos = Vector:new({ x = spawn_x, y = spawn_y }),
			vel = Vector:new({ x = velocity_x, y = velocity_y }),
		})
	end,

	update = function(self)
		WorldObject.update(self)
		local layer = self.layer
		if not layer or not self.spawn_timer or not self.spawn_timer:hasElapsed(self.spawn_interval_ms or 60) then
			return
		end
		local live_balls = self:getLiveBallCount()
		local remaining = max(0, (self.max_space_balls or 0) - live_balls)
		if remaining <= 0 then
			return
		end
		local burst = min(self.spawn_per_tick or 1, remaining)
		for _ = 1, burst do
			self:spawnBall()
		end
	end,
})

return SpaceBallSpawner
