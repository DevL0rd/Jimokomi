local Vector = include("src/Engine/Math/Vector.lua")
local WorldObjectLifecycle = include("src/Engine/Objects/WorldObject/Lifecycle.lua")

local PhysicsBody = {
	vel = Vector:new(),
	accel = Vector:new(),
	angle = 0,
	angular_vel = 0,
	angular_accel = 0,
	angular_damping = 6,
	friction = 10,
	ignore_physics = false,
	ignore_gravity = false,
	ignore_friction = false,
	body_type = nil,
	mass = 1,
	physics_material_friction = 0.2,
	physics_material_restitution = 0.0,
	physics_sleep_enabled = true,
	physics_sleeping = false,
	physics_sleep_idle_frames = 0,
	physics_sleep_frames = 10,
	physics_sleep_velocity_threshold = 2.0,
	physics_sleep_accel_threshold = 0.01,
	physics_sleep_move_threshold = 0.2,
	physics_sleep_contact_velocity_threshold = 4.0,
	physics_sleep_contact_frames = 6,
	physics_sleep_angular_velocity_threshold = 0.5,
	physics_sleep_angular_accel_threshold = 0.05,
	physics_sleep_contact_angular_velocity_threshold = 1.0,
	fixed_rotation = false,
}

PhysicsBody.mixin = function(self)
	if self._physics_body_mixin_applied then
		return
	end
	self._physics_body_mixin_applied = true
	for key, value in pairs(PhysicsBody) do
		if key ~= "mixin" and key ~= "init" and self[key] == nil then
			self[key] = value
		end
	end
end

PhysicsBody.init = function(self)
	PhysicsBody.mixin(self)
	WorldObjectLifecycle.initPhysicsBody(self)
end

PhysicsBody.cloneVector = WorldObjectLifecycle.cloneVector
PhysicsBody.canSleepPhysics = WorldObjectLifecycle.canSleepPhysics
PhysicsBody.canRotatePhysics = WorldObjectLifecycle.canRotatePhysics
PhysicsBody.isRotationLocked = WorldObjectLifecycle.isRotationLocked
PhysicsBody.setRotationLocked = WorldObjectLifecycle.setRotationLocked
PhysicsBody.isPhysicsSleeping = WorldObjectLifecycle.isPhysicsSleeping
PhysicsBody.sleepPhysics = WorldObjectLifecycle.sleepPhysics
PhysicsBody.wakePhysics = WorldObjectLifecycle.wakePhysics
PhysicsBody.updatePhysicsSleepState = WorldObjectLifecycle.updatePhysicsSleepState

return PhysicsBody
