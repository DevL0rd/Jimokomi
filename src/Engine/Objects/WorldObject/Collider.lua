local Vector = include("src/Engine/Math/Vector.lua")

local WorldObjectCollider = {}

local function draw_debug_cross(gfx, x, y, radius, color)
	radius = radius or 2
	gfx:line(x - radius, y, x + radius, y, color)
	gfx:line(x, y - radius, x, y + radius, color)
end

local function draw_rotation_indicator(self, color)
	if not self.layer or not self.layer.gfx or self:isRotationLocked() then
		return
	end
	local angle = -(self.angle or 0)
	local radius = max(4, self.getRadius and self:getRadius() or max(self:getHalfWidth(), self:getHalfHeight()))
	local x1 = self.pos.x
	local y1 = self.pos.y
	local x2 = x1 + cos(angle) * radius
	local y2 = y1 + sin(angle) * radius
	self.layer.gfx:line(x1, y1, x2, y2, color)
end

local function refresh_entity_buckets(self)
	if self.layer and self.layer.refreshEntityBuckets then
		self.layer:refreshEntityBuckets(self)
	end
end

local function draw_cached_shape(self, color, fill, options)
	if not self.layer or not self.layer.gfx then
		return
	end
	local gfx = self.layer.gfx
	options = options or {}
	local cache_tag = options.cache_tag or (fill and "shape.fill" or "shape.stroke")
	if self:isCircleShape() then
		local radius = self:getRadius()
		local anchor_x = self.pos.x - radius
		local anchor_y = self.pos.y - radius
		local cache_key = (fill and "shape_fill" or "shape_stroke") ..
			":circle:" .. tostr(radius) .. ":" .. tostr(color)
		gfx:drawSharedSurfaceStampLayer(cache_key, anchor_x, anchor_y, function(target)
			local cx = radius
			local cy = radius
			if fill then
				target:circfill(cx, cy, radius, color)
			else
				target:circ(cx, cy, radius, color)
			end
		end, {
			w = radius * 2 + 1,
			h = radius * 2 + 1,
			cache_tag = cache_tag,
			cache_profile_key = cache_tag .. ":circle:" .. (fill and "fill" or "stroke"),
			cache_profile_signature = table.concat({
				tostr(radius),
				tostr(color),
			}, ":"),
		})
		return
	end
	if self:isRectShape() then
		local top_left = self:getTopLeft()
		local width = self:getWidth()
		local height = self:getHeight()
		local cache_key = (fill and "shape_fill" or "shape_stroke") ..
			":rect:" .. tostr(width) .. ":" .. tostr(height) .. ":" .. tostr(color)
		gfx:drawSharedSurfaceStampLayer(cache_key, top_left.x, top_left.y, function(target)
			if fill then
				target:rectfill(0, 0, width - 1, height - 1, color)
			else
				target:rect(0, 0, width - 1, height - 1, color)
			end
		end, {
			w = width,
			h = height,
			cache_tag = cache_tag,
			cache_profile_key = cache_tag .. ":rect:" .. (fill and "fill" or "stroke"),
			cache_profile_signature = table.concat({
				tostr(width),
				tostr(height),
				tostr(color),
			}, ":"),
		})
	end
end

local function draw_cached_debug_overlay(self, color)
	if not self.layer or not self.layer.gfx then
		return
	end
	local gfx = self.layer.gfx
	local slot_names = self.getSlotNames and (self:getSlotNames() or {}) or {}
	if #slot_names <= 0 then
		return
	end
	local min_x = self.pos.x - 3
	local min_y = self.pos.y - 3
	local max_x = self.pos.x + 3
	local max_y = self.pos.y + 3
	local signature = "overlay"
	for i = 1, #slot_names do
		local slot_name = slot_names[i]
		local slot = self:getSlot(slot_name)
		if slot then
			local slot_pos = self:getSlotWorldPosition(slot_name)
			local slot_color = (slot.enabled ~= false and self.transform.slots_enabled ~= false) and 11 or 2
			local text_w = #slot_name * 4 + 2
			min_x = min(min_x, slot_pos.x - 3)
			min_y = min(min_y, slot_pos.y - 3)
			max_x = max(max_x, slot_pos.x + text_w + 6)
			max_y = max(max_y, slot_pos.y + 4)
			signature ..= "|" .. slot_name ..
				":" .. flr(slot_pos.x - self.pos.x + 0.5) ..
				":" .. flr(slot_pos.y - self.pos.y + 0.5) ..
				":" .. tostr(slot_color)
		end
	end
	local anchor_x = flr(min_x)
	local anchor_y = flr(min_y)
	local width = max(1, flr(max_x - anchor_x + 1))
	local height = max(1, flr(max_y - anchor_y + 1))
	local shared_identity = self.getSharedCacheIdentity and self:getSharedCacheIdentity() or tostr(self._type or "object")
	local cache_key = "collider_debug_overlay:" ..
		shared_identity .. ":" .. tostr(color) .. ":" .. signature
	gfx:drawCachedLayer(cache_key, anchor_x, anchor_y, function(target)
		local cx = self.pos.x - anchor_x
		local cy = self.pos.y - anchor_y
		for i = 1, #slot_names do
			local slot_name = slot_names[i]
			local slot = self:getSlot(slot_name)
			if slot then
				local slot_pos = self:getSlotWorldPosition(slot_name)
				local slot_color = (slot.enabled ~= false and self.transform.slots_enabled ~= false) and 11 or 2
				local sx = slot_pos.x - anchor_x
				local sy = slot_pos.y - anchor_y
				if slot_name ~= "origin" then
					target:line(cx, cy, sx, sy, slot_color)
				end
				target:circ(sx, sy, 2, slot_color)
				draw_debug_cross(target, sx, sy, 1, slot_color)
			end
		end
	end, {
		w = width,
		h = height,
		cache_tag = self.debug_cache_tag or "debug.collider.overlay",
		cache_profile_key = "debug.collider.overlay:" .. shared_identity,
		cache_profile_signature = table.concat({
			tostr(#slot_names),
			signature,
		}, ":"),
	})
end

WorldObjectCollider.getCollider = function(self)
	return self.collider
end

WorldObjectCollider.setShape = function(self, shape)
	if self.collider then
		self.collider:setShape(shape)
	else
		self.shape = shape
	end
	refresh_entity_buckets(self)
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
	refresh_entity_buckets(self)
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
	refresh_entity_buckets(self)
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
	draw_cached_shape(self, color, false)
end

WorldObjectCollider.fillShape = function(self, color)
	draw_cached_shape(self, color, true)
end

WorldObjectCollider.draw_debug = function(self)
	if not self.layer then
		return
	end
	local collider = self.collider
	local color = 8
	if collider and not collider:isEnabled() then
		color = 5
	elseif self:isTrigger() then
		color = 10
	end
	draw_cached_shape(self, color, false, {
		cache_tag = "debug.collider.shape",
	})
	if collider then
		draw_cached_debug_overlay(self, color)
	else
		draw_cached_debug_overlay(self, color)
	end
	draw_rotation_indicator(self, color)
end

return WorldObjectCollider
