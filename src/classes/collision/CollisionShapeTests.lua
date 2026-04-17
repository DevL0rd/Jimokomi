local Vector = include("src/classes/Vector.lua")

local CollisionShapeTests = {}

CollisionShapeTests.boundsOverlap = function(self, entity_a, entity_b)
	local a_half_width = entity_a:isCircleShape() and entity_a:getRadius() or entity_a:getHalfWidth()
	local a_half_height = entity_a:isCircleShape() and entity_a:getRadius() or entity_a:getHalfHeight()
	local b_half_width = entity_b:isCircleShape() and entity_b:getRadius() or entity_b:getHalfWidth()
	local b_half_height = entity_b:isCircleShape() and entity_b:getRadius() or entity_b:getHalfHeight()

	return abs(entity_a.pos.x - entity_b.pos.x) <= a_half_width + b_half_width and
		abs(entity_a.pos.y - entity_b.pos.y) <= a_half_height + b_half_height
end

CollisionShapeTests.circleToCircle = function(self, entity_a, entity_b)
	local dx = entity_a.pos.x - entity_b.pos.x
	local dy = entity_a.pos.y - entity_b.pos.y
	local distance_sq = dx * dx + dy * dy
	local min_distance = entity_a:getRadius() + entity_b:getRadius()
	local min_distance_sq = min_distance * min_distance

	if distance_sq >= min_distance_sq then
		return false
	end
	if distance_sq == 0 then
		return Vector:new({ x = min_distance, y = 0 })
	end

	local distance = sqrt(distance_sq)
	local overlap = min_distance - distance
	return Vector:new({
		x = (dx / distance) * overlap,
		y = (dy / distance) * overlap,
	})
end

CollisionShapeTests.rectToRect = function(self, entity_a, entity_b)
	local dx = entity_a.pos.x - entity_b.pos.x
	local dy = entity_a.pos.y - entity_b.pos.y
	local overlap_x = entity_a:getHalfWidth() + entity_b:getHalfWidth() - abs(dx)
	local overlap_y = entity_a:getHalfHeight() + entity_b:getHalfHeight() - abs(dy)

	if overlap_x <= 0 or overlap_y <= 0 then
		return false
	end
	if overlap_x < overlap_y then
		return Vector:new({
			x = dx >= 0 and overlap_x or -overlap_x,
			y = 0,
		})
	end
	return Vector:new({
		x = 0,
		y = dy >= 0 and overlap_y or -overlap_y,
	})
end

CollisionShapeTests.circleToRect = function(self, circle, rect)
	local rect_left = rect.pos.x - rect:getHalfWidth()
	local rect_right = rect.pos.x + rect:getHalfWidth()
	local rect_top = rect.pos.y - rect:getHalfHeight()
	local rect_bottom = rect.pos.y + rect:getHalfHeight()
	local closest_x = max(rect_left, min(circle.pos.x, rect_right))
	local closest_y = max(rect_top, min(circle.pos.y, rect_bottom))
	local dx = circle.pos.x - closest_x
	local dy = circle.pos.y - closest_y
	local distance_sq = dx * dx + dy * dy
	local radius = circle:getRadius()

	if distance_sq >= radius * radius then
		return false
	end
	if distance_sq == 0 then
		local push_left = circle.pos.x - rect_left
		local push_right = rect_right - circle.pos.x
		local push_up = circle.pos.y - rect_top
		local push_down = rect_bottom - circle.pos.y
		local min_push = min(min(push_left, push_right), min(push_up, push_down))

		if min_push == push_left then
			return Vector:new({ x = radius, y = 0 })
		elseif min_push == push_right then
			return Vector:new({ x = -radius, y = 0 })
		elseif min_push == push_up then
			return Vector:new({ x = 0, y = radius })
		end
		return Vector:new({ x = 0, y = -radius })
	end

	local distance = sqrt(distance_sq)
	local overlap = radius - distance
	return Vector:new({
		x = (dx / distance) * overlap,
		y = (dy / distance) * overlap,
	})
end

CollisionShapeTests.entityToEntity = function(self, entity_a, entity_b)
	if entity_a:isCircleShape() then
		if entity_b:isCircleShape() then
			return self:circleToCircle(entity_a, entity_b)
		end
		if entity_b:isRectShape() then
			return self:circleToRect(entity_a, entity_b)
		end
	elseif entity_a:isRectShape() then
		if entity_b:isRectShape() then
			return self:rectToRect(entity_a, entity_b)
		end
		if entity_b:isCircleShape() then
			local push = self:circleToRect(entity_b, entity_a)
			if push then
				return Vector:new({ x = -push.x, y = -push.y })
			end
		end
	end

	return false
end

return CollisionShapeTests
