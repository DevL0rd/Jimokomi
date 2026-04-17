local Vector = include("src/Engine/Math/Vector.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local Transform = include("src/Engine/World/Transform.lua")
local Collider = include("src/Engine/World/Collider.lua")

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

WorldObjectLifecycle.getWorld = function(self)
	return self.layer and self.layer.world or nil
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

	self.collider = Collider:new({
		owner = self,
		shape = self.shape,
		collision_layer = self.collision_layer,
		collision_mask = self.collision_mask,
		is_trigger = self.is_trigger,
		resolve_dynamic = self.resolve_entity_collisions,
		enabled = not self.ignore_collisions,
	})

	self.vel = WorldObjectLifecycle.cloneVector(self.vel)
	self.accel = WorldObjectLifecycle.cloneVector(self.accel)
	self.timer = Timer:new()
	self.percent_expired = 0

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
end

WorldObjectLifecycle.needsUpdate = function(self)
	return self.update ~= WorldObjectLifecycle.update or self.lifetime ~= -1
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
		local vx = self.vel.x * 0.25
		local vy = self.vel.y * 0.25
		self.layer.gfx:line(self.pos.x, self.pos.y, self.pos.x + vx, self.pos.y + vy, 8)
	end
end

WorldObjectLifecycle.needsDraw = function(self)
	return self.draw ~= WorldObjectLifecycle.draw or self.fill ~= nil or self.stroke ~= nil or
		self.fill_color > -1 or self.stroke_color > -1 or self.debug
end

WorldObjectLifecycle.unInit = function(self)
end

return WorldObjectLifecycle
