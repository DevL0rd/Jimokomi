local Vector = include("src/Engine/Math/Vector.lua")

local WorldObjectCollider = {}

WorldObjectCollider.getCollider = function(self)
	return self.collider
end

WorldObjectCollider.setShape = function(self, shape)
	if self.collider then
		self.collider:setShape(shape)
	else
		self.shape = shape
	end
end

WorldObjectCollider.setCircleShape = function(self, radius)
	self:setShape({
		kind = "circle",
		r = radius,
	})
end

WorldObjectCollider.setRectShape = function(self, width, height)
	self:setShape({
		kind = "rect",
		w = width,
		h = height,
	})
end

WorldObjectCollider.getShapeKind = function(self)
	return self.collider and self.collider:getShapeKind() or nil
end

WorldObjectCollider.isCircleShape = function(self)
	return self.collider and self.collider:isCircle() or false
end

WorldObjectCollider.isRectShape = function(self)
	return self.collider and self.collider:isRect() or false
end

WorldObjectCollider.getRadius = function(self)
	return self.collider and self.collider:getRadius() or 0
end

WorldObjectCollider.getWidth = function(self)
	return self.collider and self.collider:getWidth() or 0
end

WorldObjectCollider.getHeight = function(self)
	return self.collider and self.collider:getHeight() or 0
end

WorldObjectCollider.getHalfWidth = function(self)
	return self.collider and self.collider:getHalfWidth() or 0
end

WorldObjectCollider.getHalfHeight = function(self)
	return self.collider and self.collider:getHalfHeight() or 0
end

WorldObjectCollider.getTopLeft = function(self)
	return Vector:new({
		x = self.pos.x - self:getHalfWidth(),
		y = self.pos.y - self:getHalfHeight(),
	})
end

WorldObjectCollider.getCollisionLayer = function(self)
	return self.collider and self.collider:getCollisionLayer() or "default"
end

WorldObjectCollider.setCollisionLayer = function(self, layer_name)
	if self.collider then
		self.collider:setCollisionLayer(layer_name)
	else
		self.collision_layer = layer_name or "default"
	end
end

WorldObjectCollider.allowsCollisionLayer = function(self, layer_name)
	return self.collider and self.collider:allowsCollisionLayer(layer_name) or true
end

WorldObjectCollider.setCollisionMask = function(self, mask)
	if self.collider then
		self.collider:setCollisionMask(mask)
	else
		self.collision_mask = mask
	end
end

WorldObjectCollider.isTrigger = function(self)
	return self.collider and self.collider:isTriggerCollider() or false
end

WorldObjectCollider.canCollideWith = function(self, other)
	if self.ignore_collisions or (other and other.ignore_collisions) then
		return false
	end
	if not self.collider or not other or not other.getCollider then
		return false
	end
	return self.collider:canCollideWith(other:getCollider())
end

WorldObjectCollider.strokeShape = function(self, color)
	if not self.layer then
		return
	end
	if self:isCircleShape() then
		self.layer.gfx:circ(self.pos.x, self.pos.y, self:getRadius(), color)
		return
	end
	if self:isRectShape() then
		local top_left = self:getTopLeft()
		self.layer.gfx:rect(
			top_left.x,
			top_left.y,
			top_left.x + self:getWidth() - 1,
			top_left.y + self:getHeight() - 1,
			color
		)
	end
end

WorldObjectCollider.fillShape = function(self, color)
	if not self.layer then
		return
	end
	if self:isCircleShape() then
		self.layer.gfx:circfill(self.pos.x, self.pos.y, self:getRadius(), color)
		return
	end
	if self:isRectShape() then
		local top_left = self:getTopLeft()
		self.layer.gfx:rectfill(
			top_left.x,
			top_left.y,
			top_left.x + self:getWidth() - 1,
			top_left.y + self:getHeight() - 1,
			color
		)
	end
end

WorldObjectCollider.draw_debug = function(self)
	self:strokeShape(8)
end

return WorldObjectCollider
