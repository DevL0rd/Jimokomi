local Vector = include("src/Engine/Math/Vector.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local Transform = include("src/Engine/World/Transform.lua")
local Collider = include("src/Engine/World/Collider.lua")
local SharedVisualCache = include("src/Engine/Mixins/SharedVisualCache.lua")

local WorldObjectLifecycle = {}

WorldObjectLifecycle.cloneVector = function(value)
	return Vector:new({
		x = value and value.x or 0,
		y = value and value.y or 0,
	})
end

WorldObjectLifecycle.getAssets = function(self)
	return nil
end

WorldObjectLifecycle.getSpriteRotationAngle = function(self)
	return -(self.angle or 0)
end

WorldObjectLifecycle.getSpriteRotationBucketCount = function(self)
	return SharedVisualCache.resolveRotationBucketCount(self, self:getSpriteRotationAngle(), self.rotation_bucket_count or 32)
end

WorldObjectLifecycle.drawSharedSprite = function(self, sprite_id, x, y, width, height, options)
	local layer = self.layer
	local gfx = layer and layer.gfx or nil
	if not gfx or sprite_id == nil or sprite_id < 0 then
		return nil
	end
	width = width or 16
	height = height or 16
	options = options or {}
	local flip_x = options.flip_x == true
	local flip_y = options.flip_y == true
	local cache_key = options.cache_key or table.concat({
		"world_object_sprite",
		tostr(self.getSharedCacheIdentity and self:getSharedCacheIdentity() or self._type or "world_object"),
		tostr(sprite_id),
		tostr(flip_x),
		tostr(flip_y),
		tostr(width),
		tostr(height),
	}, ":")
	return gfx:drawCachedSpriteLayer(
		cache_key,
		sprite_id,
		x,
		y,
		flip_x,
		flip_y,
		{
			w = width,
			h = height,
			cache_tag = options.cache_tag or "object.sprite",
			cache_profile_key = options.cache_profile_key or ("object.sprite:" .. tostr(self.getSharedCacheIdentity and self:getSharedCacheIdentity() or self._type or "world_object")),
			cache_profile_signature = options.cache_profile_signature or table.concat({
				tostr(width),
				tostr(height),
				tostr(flip_x),
				tostr(flip_y),
			}, ":"),
			rotation_angle = options.rotation_angle ~= nil and options.rotation_angle or self:getSpriteRotationAngle(),
			rotation_bucket_count = options.rotation_bucket_count or self:getSpriteRotationBucketCount(),
			entry_ttl_ms = SharedVisualCache.resolveEntryTtlMs(self, options.entry_ttl_ms, self.sprite_cache_entry_ttl_ms or 30000),
			rotation_entry_ttl_ms = SharedVisualCache.resolveRotationEntryTtlMs(
				self,
				options.rotation_entry_ttl_ms,
				options.entry_ttl_ms,
				self.sprite_rotation_cache_entry_ttl_ms or self.sprite_cache_entry_ttl_ms or 30000
			),
		}
	)
end

WorldObjectLifecycle.getWorld = function(self)
	return self.layer and self.layer.world or nil
end

WorldObjectLifecycle.markSpatialDirty = function(self)
	local layer = self and self.layer or nil
	if layer and layer.markSpatialIndexDirty then
		layer:markSpatialIndexDirty(self)
	end
end

WorldObjectLifecycle.initPhysicsBody = function(self)
	self.collider = Collider:new({
		owner = self,
		shape = self.shape,
		body_type = self.body_type or (self.ignore_physics == true and "static" or "dynamic"),
		collision_layer = self.collision_layer,
		collision_mask = self.collision_mask,
		is_trigger = self.is_trigger,
		resolve_dynamic = self.resolve_entity_collisions,
		enabled = not self.ignore_collisions,
		mass = self.mass or 1,
		friction = self.physics_material_friction or 0.2,
		restitution = self.physics_material_restitution or 0.0,
		fixed_rotation = self.fixed_rotation == true,
	})

	self.vel = WorldObjectLifecycle.cloneVector(self.vel)
	self.accel = WorldObjectLifecycle.cloneVector(self.accel)
	self.angle = self.angle or 0
	self.angular_vel = self.angular_vel or 0
	self.angular_accel = self.angular_accel or 0
	self.angular_damping = self.angular_damping or 6
	self.physics_sleeping = self.physics_sleeping == true
	self.physics_sleep_idle_frames = self.physics_sleep_idle_frames or 0
	self._wake_physics_requested = false
end

WorldObjectLifecycle.canSleepPhysics = function(self)
	if self.physics_sleep_enabled == false then
		return false
	end
	if self.ignore_physics == true then
		return false
	end
	if self.parent ~= nil then
		return false
	end
	return true
end

WorldObjectLifecycle.canRotatePhysics = function(self)
	if self.ignore_physics == true then
		return false
	end
	local collider = self.getCollider and self:getCollider() or nil
	if not collider then
		return self.fixed_rotation ~= true
	end
	return collider:getInverseInertia() > 0
end

WorldObjectLifecycle.isRotationLocked = function(self)
	local collider = self.getCollider and self:getCollider() or nil
	if collider and collider.isFixedRotation then
		return collider:isFixedRotation()
	end
	return self.fixed_rotation == true
end

WorldObjectLifecycle.setRotationLocked = function(self, locked)
	self.fixed_rotation = locked == true
	local collider = self.getCollider and self:getCollider() or nil
	if collider and collider.setFixedRotation then
		collider:setFixedRotation(locked == true)
	end
	if locked == true then
		self.angular_vel = 0
		self.angular_accel = 0
	end
end

WorldObjectLifecycle.isPhysicsSleeping = function(self)
	return self.physics_sleeping == true
end

WorldObjectLifecycle.sleepPhysics = function(self)
	if not self:canSleepPhysics() or self.physics_sleeping == true then
		return false
	end
	self.physics_sleeping = true
	self.physics_sleep_idle_frames = self.physics_sleep_frames or 12
	self._wake_physics_requested = false
	if self.vel then
		self.vel.x = 0
		self.vel.y = 0
	end
	self.angular_vel = 0
	self.angular_accel = 0
	self:markSpatialDirty()
	return true
end

WorldObjectLifecycle.wakePhysics = function(self)
	if self.physics_sleeping ~= true and self._wake_physics_requested ~= true then
		return false
	end
	self.physics_sleeping = false
	self.physics_sleep_idle_frames = 0
	self._wake_physics_requested = false
	local layer = self.layer
	if layer then
		layer.entity_static_spatial_dirty = true
		layer.collidable_static_spatial_dirty = true
	end
	self:markSpatialDirty()
	return true
end

WorldObjectLifecycle.updatePhysicsSleepState = function(self)
	if not self:canSleepPhysics() then
		self.physics_sleeping = false
		self.physics_sleep_idle_frames = 0
		self._wake_physics_requested = false
		return false
	end
	if self._wake_physics_requested == true then
		self:wakePhysics()
		return false
	end

	local vel = self.vel or {}
	local accel = self.accel or {}
	local speed_x = abs(vel.x or 0)
	local speed_y = abs(vel.y or 0)
	local accel_x = abs(accel.x or 0)
	local accel_y = abs(accel.y or 0)
	local angular_speed = abs(self.angular_vel or 0)
	local angular_accel = abs(self.angular_accel or 0)
	local sleep_speed = self.physics_sleep_velocity_threshold or 0.05
	local sleep_accel = self.physics_sleep_accel_threshold or 0.01
	local sleep_move = self.physics_sleep_move_threshold or 0.05
	local sleep_angular_speed = self.physics_sleep_angular_velocity_threshold or 0.5
	local sleep_angular_accel = self.physics_sleep_angular_accel_threshold or 0.05
	local collisions = self.collisions
	local collision_count = collisions and #collisions or 0
	local start_x = self._physics_step_start_x or self.pos.x or 0
	local start_y = self._physics_step_start_y or self.pos.y or 0
	local end_x = self.pos and self.pos.x or start_x
	local end_y = self.pos and self.pos.y or start_y
	local moved_x = abs(end_x - start_x)
	local moved_y = abs(end_y - start_y)
	if collision_count > 0 then
		sleep_speed = max(sleep_speed, self.physics_sleep_contact_velocity_threshold or 4.0)
		sleep_move = max(sleep_move, 999999)
		sleep_angular_speed = max(sleep_angular_speed, self.physics_sleep_contact_angular_velocity_threshold or 1.0)
	end

	if speed_x > sleep_speed or speed_y > sleep_speed or accel_x > sleep_accel or accel_y > sleep_accel or
		moved_x > sleep_move or moved_y > sleep_move or
		angular_speed > sleep_angular_speed or angular_accel > sleep_angular_accel then
		self.physics_sleep_idle_frames = 0
		self.physics_sleeping = false
		return false
	end

	self.physics_sleep_idle_frames = (self.physics_sleep_idle_frames or 0) + 1
	local required_frames = collision_count > 0 and (self.physics_sleep_contact_frames or 6) or (self.physics_sleep_frames or 12)
	if self.physics_sleep_idle_frames >= required_frames then
		self:sleepPhysics()
		return true
	end
	return false
end

WorldObjectLifecycle.init = function(self)
	local initial_pos = self.pos or Vector:new()
	self.transform = Transform:new({
		owner = self,
		position = initial_pos,
		local_position = self.slot_offset,
		slot_defs = self.slot_defs,
		slots_enabled = self.slots_enabled ~= false,
		default_slot_destroy_policy = self.default_slot_destroy_policy,
	})
	self.pos = self.transform.position
	self.slot_offset = self.transform:getSlotOffset()
	self:initPhysicsBody()
	self.timer = Timer:new()
	self.percent_expired = 0
	self.attachment_nodes = self.attachment_nodes or {}
	self._visual_runtime_initialized = false

	if self.layer and self.layer.engine and self.object_id == nil and self.layer.engine.nextObjectId then
		self.object_id = self.layer.engine:nextObjectId()
	end

	if self.parent then
		self:attachTo(self.parent, self.parent_slot, {
			offset = self.slot_offset,
		})
	end
	if self.layer then
		self.layer:add(self)
	end
end

WorldObjectLifecycle.update = function(self)
	if self.lifetime ~= -1 then
		self.percent_expired = self.timer:elapsed() / self.lifetime
		if self.timer:hasElapsed(self.lifetime) then
			self:destroy()
		end
	end
	if self.updateAttachmentNodes then
		self:updateAttachmentNodes()
	end
end

WorldObjectLifecycle.needsUpdate = function(self)
	return self.update ~= WorldObjectLifecycle.update or self.lifetime ~= -1
end

WorldObjectLifecycle.isSpatialStatic = function(self)
	if self.parent ~= nil then
		return false
	end
	if self.ignore_physics ~= true then
		return false
	end
	if self.lifetime ~= -1 then
		return false
	end
	if self.update ~= WorldObjectLifecycle.update then
		return false
	end
	if self.needsUpdate ~= WorldObjectLifecycle.needsUpdate then
		return false
	end
	if self.vel and (self.vel.x ~= 0 or self.vel.y ~= 0) then
		return false
	end
	if self.accel and (self.accel.x ~= 0 or self.accel.y ~= 0) then
		return false
	end
	return true
end

WorldObjectLifecycle.draw = function(self)
	if self.fill then
		self:fill()
	elseif self.fill_color > -1 then
		self:fillShape(self.fill_color)
	end
	if self.stroke then
		self:stroke()
	elseif self.stroke_color > -1 then
		self:strokeShape(self.stroke_color)
	end
	if self.debug then
		self:draw_debug()
		local vel = self.vel or nil
		local speed_sq = vel and ((vel.x or 0) * (vel.x or 0) + (vel.y or 0) * (vel.y or 0)) or 0
		if speed_sq > 4 then
			local vx = vel.x * 0.25
			local vy = vel.y * 0.25
			local sx, sy = self.layer.camera:layerToScreenXY(self.pos.x, self.pos.y)
			self.layer.gfx:screenLine(sx, sy, sx + vx, sy + vy, 8)
		end
	end
	if self.drawAttachmentNodes then
		if not (self.drawSingleAttachmentSpriteFast and self:drawSingleAttachmentSpriteFast()) then
			self:drawAttachmentNodes()
		end
	end
end

WorldObjectLifecycle.needsDraw = function(self)
	local has_attachment_nodes = self.attachment_nodes and #self.attachment_nodes > 0
	local has_visual_sprite = self.visual and self.visual.getSprite and self.visual:getSprite() ~= nil
	return self.draw ~= WorldObjectLifecycle.draw or self.fill ~= nil or self.stroke ~= nil or
		self.fill_color > -1 or self.stroke_color > -1 or self.debug or has_attachment_nodes or has_visual_sprite
end

WorldObjectLifecycle.unInit = function(self)
	if self.destroyAttachmentNodes then
		self:destroyAttachmentNodes()
	end
end

return WorldObjectLifecycle
