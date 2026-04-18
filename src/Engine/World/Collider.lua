local Class = include("src/Engine/Core/Class.lua")

local Collider = Class:new({
	_type = "Collider",
	owner = nil,
	shape = nil,
	body_type = "dynamic",
	collision_layer = "default",
	collision_mask = nil,
	is_trigger = false,
	resolve_dynamic = false,
	enabled = true,
	mass = 1,
	friction = 0.2,
	restitution = 0.0,
	fixed_rotation = false,

	init = function(self)
		self.body_type = self.body_type or "dynamic"
		self.mass = max(0.0001, self.mass or 1)
		self.friction = max(0, self.friction or 0.2)
		self.restitution = mid(0, self.restitution or 0.0, 1)
		self.fixed_rotation = self.fixed_rotation == true
		if self.shape then
			self:setShape(self.shape)
		end
	end,

	setShape = function(self, shape)
		if not shape then
			self.shape = nil
			return
		end
		self.shape = {
			kind = shape.kind,
			r = shape.r,
			w = shape.w,
			h = shape.h,
		}
	end,

	setCircleShape = function(self, radius)
		self:setShape({
			kind = "circle",
			r = radius,
		})
	end,

	setRectShape = function(self, width, height)
		self:setShape({
			kind = "rect",
			w = width,
			h = height,
		})
	end,

	getShapeKind = function(self)
		return self.shape and self.shape.kind or nil
	end,

	isCircle = function(self)
		return self:getShapeKind() == "circle"
	end,

	isRect = function(self)
		return self:getShapeKind() == "rect"
	end,

	getRadius = function(self)
		if self:isCircle() then
			return self.shape.r or 0
		end
		if self:isRect() then
			return max(self.shape.w or 0, self.shape.h or 0) * 0.5
		end
		return 0
	end,

	getWidth = function(self)
		if self:isRect() then
			return self.shape.w or 0
		end
		if self:isCircle() then
			return (self.shape.r or 0) * 2
		end
		return 0
	end,

	getHeight = function(self)
		if self:isRect() then
			return self.shape.h or 0
		end
		if self:isCircle() then
			return (self.shape.r or 0) * 2
		end
		return 0
	end,

	getHalfWidth = function(self)
		return self:getWidth() * 0.5
	end,

	getHalfHeight = function(self)
		return self:getHeight() * 0.5
	end,

	setEnabled = function(self, enabled)
		self.enabled = enabled ~= false
		local owner = self.owner
		if owner and owner.layer and owner.layer.refreshEntityBuckets then
			owner.layer:refreshEntityBuckets(owner)
		end
	end,

	isEnabled = function(self)
		return self.enabled ~= false
	end,

	setBodyType = function(self, body_type)
		self.body_type = body_type or "dynamic"
	end,

	getBodyType = function(self)
		return self.body_type or "dynamic"
	end,

	isDynamicBody = function(self)
		return self:getBodyType() == "dynamic"
	end,

	isStaticBody = function(self)
		return self:getBodyType() == "static"
	end,

	isKinematicBody = function(self)
		return self:getBodyType() == "kinematic"
	end,

	setMass = function(self, mass)
		self.mass = max(0.0001, mass or 1)
	end,

	getMass = function(self)
		return max(0.0001, self.mass or 1)
	end,

	getInverseMass = function(self)
		if not self:isDynamicBody() then
			return 0
		end
		return 1 / self:getMass()
	end,

	setFixedRotation = function(self, fixed_rotation)
		self.fixed_rotation = fixed_rotation == true
	end,

	isFixedRotation = function(self)
		return self.fixed_rotation == true
	end,

	getMomentOfInertia = function(self)
		local mass = self:getMass()
		if self:isCircle() then
			local radius = self:getRadius()
			return 0.5 * mass * radius * radius
		end
		if self:isRect() then
			local width = self:getWidth()
			local height = self:getHeight()
			return (mass * (width * width + height * height)) / 12
		end
		return mass
	end,

	getInverseInertia = function(self)
		if not self:isDynamicBody() or self:isFixedRotation() then
			return 0
		end
		local inertia = self:getMomentOfInertia()
		if inertia <= 0 then
			return 0
		end
		return 1 / inertia
	end,

	setFriction = function(self, friction)
		self.friction = max(0, friction or 0)
	end,

	getFriction = function(self)
		return max(0, self.friction or 0)
	end,

	setRestitution = function(self, restitution)
		self.restitution = mid(0, restitution or 0, 1)
	end,

	getRestitution = function(self)
		return mid(0, self.restitution or 0, 1)
	end,

	setCollisionLayer = function(self, layer_name)
		self.collision_layer = layer_name or "default"
	end,

	getCollisionLayer = function(self)
		return self.collision_layer or "default"
	end,

	setCollisionMask = function(self, mask)
		self.collision_mask = mask
	end,

	allowsCollisionLayer = function(self, layer_name)
		local mask = self.collision_mask
		if mask == nil then
			return true
		end
		if type(mask) == "string" then
			return mask == layer_name
		end
		if mask[layer_name] ~= nil then
			return mask[layer_name] == true
		end
		for _, accepted in pairs(mask) do
			if accepted == layer_name then
				return true
			end
		end
		return false
	end,

	canCollideWith = function(self, other)
		if not other or not self:isEnabled() or not other:isEnabled() then
			return false
		end
		return self:allowsCollisionLayer(other:getCollisionLayer()) and
			other:allowsCollisionLayer(self:getCollisionLayer())
	end,

	isTriggerCollider = function(self)
		return self.is_trigger == true
	end,

	canResolveWith = function(self, other)
		if self.resolve_dynamic ~= true or other.resolve_dynamic ~= true then
			return false
		end
		if self:isTriggerCollider() or other:isTriggerCollider() then
			return false
		end
		local self_type = self:getBodyType()
		local other_type = other:getBodyType()
		if self_type == "static" and other_type == "static" then
			return false
		end
		if self_type == "kinematic" and other_type == "kinematic" then
			return false
		end
		return true
	end,

	toSnapshot = function(self)
		return {
			shape = self.shape and {
				kind = self.shape.kind,
				r = self.shape.r,
				w = self.shape.w,
				h = self.shape.h,
			} or nil,
			collision_layer = self.collision_layer,
			collision_mask = self.collision_mask,
			body_type = self.body_type,
			is_trigger = self.is_trigger == true,
			resolve_dynamic = self.resolve_dynamic == true,
			enabled = self.enabled ~= false,
			mass = self.mass,
			friction = self.friction,
			restitution = self.restitution,
			fixed_rotation = self.fixed_rotation == true,
		}
	end,

	applySnapshot = function(self, snapshot)
		if not snapshot then
			return
		end
		self:setShape(snapshot.shape)
		self.collision_layer = snapshot.collision_layer or "default"
		self.collision_mask = snapshot.collision_mask
		self.body_type = snapshot.body_type or "dynamic"
		self.is_trigger = snapshot.is_trigger == true
		self.resolve_dynamic = snapshot.resolve_dynamic == true
		self.enabled = snapshot.enabled ~= false
		self.mass = max(0.0001, snapshot.mass or self.mass or 1)
		self.friction = max(0, snapshot.friction or self.friction or 0.2)
		self.restitution = mid(0, snapshot.restitution or self.restitution or 0.0, 1)
		self.fixed_rotation = snapshot.fixed_rotation == true
	end,
})

return Collider
