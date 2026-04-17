local Vector = include("src/classes/Vector.lua")

local LARGE_NUMBER = 32767

local RaycastShapes = {}

RaycastShapes.rayToLayer = function(self, ray, layer_obj)
	local t_x_min, t_x_max
	local t_y_min, t_y_max

	if ray.vec.x == 0 then
		if ray.pos.x < layer_obj.x or ray.pos.x > layer_obj.x + layer_obj.w then
			return false
		end
		t_x_min = -LARGE_NUMBER
		t_x_max = LARGE_NUMBER
	else
		t_x_min = (layer_obj.x - ray.pos.x) / ray.vec.x
		t_x_max = (layer_obj.x + layer_obj.w - ray.pos.x) / ray.vec.x
		if t_x_min > t_x_max then
			t_x_min, t_x_max = t_x_max, t_x_min
		end
	end

	if ray.vec.y == 0 then
		if ray.pos.y < layer_obj.y or ray.pos.y > layer_obj.y + layer_obj.h then
			return false
		end
		t_y_min = -LARGE_NUMBER
		t_y_max = LARGE_NUMBER
	else
		t_y_min = (layer_obj.y - ray.pos.y) / ray.vec.y
		t_y_max = (layer_obj.y + layer_obj.h - ray.pos.y) / ray.vec.y
		if t_y_min > t_y_max then
			t_y_min, t_y_max = t_y_max, t_y_min
		end
	end

	local t_enter = max(t_x_min, t_y_min)
	local t_exit = min(t_x_max, t_y_max)

	if t_enter > t_exit or t_exit < 0 then
		return false
	end
	if t_enter < 0 then
		return t_exit
	end
	return t_enter
end

RaycastShapes.rayToCircle = function(self, ray, obj)
	local radius = obj:getRadius()
	local l = Vector:new({ x = obj.pos.x - ray.pos.x, y = obj.pos.y - ray.pos.y })
	local tca = l:dot(ray.vec)

	if tca < 0 then
		return false
	end

	local d2 = l:dot(l) - tca * tca
	local r2 = radius * radius
	if d2 > r2 then
		return false
	end

	local thc = sqrt(r2 - d2)
	local t = tca - thc
	if t < 0 then
		t = tca + thc
	end

	return t
end

RaycastShapes.rayToRect = function(self, ray, obj)
	local t_near_x, t_far_x
	local t_near_y, t_far_y

	local left = obj.pos.x - obj:getHalfWidth()
	local right = obj.pos.x + obj:getHalfWidth()
	local top = obj.pos.y - obj:getHalfHeight()
	local bottom = obj.pos.y + obj:getHalfHeight()

	if ray.vec.x == 0 then
		if ray.pos.x < left or ray.pos.x > right then
			return false
		end
		t_near_x = -LARGE_NUMBER
		t_far_x = LARGE_NUMBER
	else
		t_near_x = (left - ray.pos.x) / ray.vec.x
		t_far_x = (right - ray.pos.x) / ray.vec.x
		if t_near_x > t_far_x then
			t_near_x, t_far_x = t_far_x, t_near_x
		end
	end

	if ray.vec.y == 0 then
		if ray.pos.y < top or ray.pos.y > bottom then
			return false
		end
		t_near_y = -LARGE_NUMBER
		t_far_y = LARGE_NUMBER
	else
		t_near_y = (top - ray.pos.y) / ray.vec.y
		t_far_y = (bottom - ray.pos.y) / ray.vec.y
		if t_near_y > t_far_y then
			t_near_y, t_far_y = t_far_y, t_near_y
		end
	end

	local t_hit_near = max(t_near_x, t_near_y)
	local t_hit_far = min(t_far_x, t_far_y)

	if t_hit_far < t_hit_near or t_hit_far < 0 then
		return false
	end

	return t_hit_near
end

return RaycastShapes
