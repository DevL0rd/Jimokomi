local CollisionShapeTests = {}

local function clamp(value, min_value, max_value)
	return max(min_value, min(max_value, value))
end

local function build_contact(push_x, push_y, contact_x, contact_y)
	local penetration = sqrt(push_x * push_x + push_y * push_y)
	if penetration <= 0 then
		return false
	end
	return {
		x = push_x,
		y = push_y,
		penetration = penetration,
		normal_x = push_x / penetration,
		normal_y = push_y / penetration,
		contact_x = contact_x,
		contact_y = contact_y,
	}
end

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
		return build_contact(min_distance, 0, entity_a.pos.x - entity_a:getRadius(), entity_a.pos.y)
	end

	local distance = sqrt(distance_sq)
	local overlap = min_distance - distance
	local nx = dx / distance
	local ny = dy / distance
	return build_contact(
		nx * overlap,
		ny * overlap,
		entity_a.pos.x - nx * entity_a:getRadius(),
		entity_a.pos.y - ny * entity_a:getRadius()
	)
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
		local nx = dx >= 0 and 1 or -1
		local contact_x = entity_a.pos.x - nx * entity_a:getHalfWidth()
		local contact_y = clamp(
			entity_b.pos.y,
			entity_a.pos.y - entity_a:getHalfHeight(),
			entity_a.pos.y + entity_a:getHalfHeight()
		)
		return build_contact(nx * overlap_x, 0, contact_x, contact_y)
	end
	local ny = dy >= 0 and 1 or -1
	local contact_x = clamp(
		entity_b.pos.x,
		entity_a.pos.x - entity_a:getHalfWidth(),
		entity_a.pos.x + entity_a:getHalfWidth()
	)
	local contact_y = entity_a.pos.y - ny * entity_a:getHalfHeight()
	return build_contact(0, ny * overlap_y, contact_x, contact_y)
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
			return build_contact(radius, 0, rect_left, clamp(circle.pos.y, rect_top, rect_bottom))
		elseif min_push == push_right then
			return build_contact(-radius, 0, rect_right, clamp(circle.pos.y, rect_top, rect_bottom))
		elseif min_push == push_up then
			return build_contact(0, radius, clamp(circle.pos.x, rect_left, rect_right), rect_top)
		end
		return build_contact(0, -radius, clamp(circle.pos.x, rect_left, rect_right), rect_bottom)
	end

	local distance = sqrt(distance_sq)
	local overlap = radius - distance
	return build_contact((dx / distance) * overlap, (dy / distance) * overlap, closest_x, closest_y)
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
				return build_contact(-push.x, -push.y, push.contact_x, push.contact_y)
			end
		end
	end

	return false
end

return CollisionShapeTests
