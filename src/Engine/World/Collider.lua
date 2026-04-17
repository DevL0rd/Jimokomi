local Class = include("src/Engine/Core/Class.lua")

local Collider = Class:new({
	_type = "Collider",
	owner = nil,
	shape = nil,
	collision_layer = "default",
	collision_mask = nil,
	is_trigger = false,
	resolve_dynamic = false,
	enabled = true,

	init = function(self)
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
		return self.resolve_dynamic == true and
			other.resolve_dynamic == true and
			not self:isTriggerCollider() and
			not other:isTriggerCollider()
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
			is_trigger = self.is_trigger == true,
			resolve_dynamic = self.resolve_dynamic == true,
			enabled = self.enabled ~= false,
		}
	end,

	applySnapshot = function(self, snapshot)
		if not snapshot then
			return
		end
		self:setShape(snapshot.shape)
		self.collision_layer = snapshot.collision_layer or "default"
		self.collision_mask = snapshot.collision_mask
		self.is_trigger = snapshot.is_trigger == true
		self.resolve_dynamic = snapshot.resolve_dynamic == true
		self.enabled = snapshot.enabled ~= false
	end,
})

return Collider
