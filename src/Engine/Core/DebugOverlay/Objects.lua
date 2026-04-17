local DebugOverlayObjects = {}

DebugOverlayObjects.getObjectPathing = function(self, object)
	if object.autonomy and object.autonomy.pathing then
		return object.autonomy.pathing
	end
	if object.flight_locomotion and object.flight_locomotion.pathing then
		return object.flight_locomotion.pathing
	end
	if object.ground_patrol_locomotion and object.ground_patrol_locomotion.pathing then
		return object.ground_patrol_locomotion.pathing
	end
	return nil
end

DebugOverlayObjects.getObjectLine = function(self, object)
	if not object then
		return nil
	end
	if object._type ~= "Player" then
		return nil
	end
	local line = (object.display_name or object._type) ..
		" (" .. flr(object.pos.x) .. "," .. flr(object.pos.y) .. ")"
	if object.display_name then
		line ..= " ctrl:" .. object.control_mode
	end
	if object.health ~= nil and object.max_health ~= nil then
		line ..= " hp:" .. object.health .. "/" .. object.max_health
	end
	if object.actions and object.actions.action then
		line ..= " act:" .. object.actions.action
	end
	if object.ai and object.ai.state then
		line ..= " st:" .. object.ai.state
	end
	if object.perception then
		if object.perception.seen_target then
			line ..= " see:" .. (object.perception.seen_target.display_name or object.perception.seen_target._type or "?")
		end
		if object.perception.heard_target then
			line ..= " hear:" .. (object.perception.heard_target.display_name or object.perception.heard_target._type or "?")
		end
	end
	local pathing = self:getObjectPathing(object)
	if pathing then
		local path_len = pathing.path and #pathing.path or 0
		line ..= " path:" .. path_len
		if pathing.last_path_info and pathing.last_path_info.iterations then
			line ..= " it:" .. pathing.last_path_info.iterations
		end
	end
	return line
end

DebugOverlayObjects.drawMarker = function(self, layer, pos, color, label)
	if not layer or not layer.gfx or not pos then
		return
	end
	layer.gfx:circ(pos.x, pos.y, 2, color)
	layer.gfx:circfill(pos.x, pos.y, 1, color)
	if label and label ~= "" then
		layer.gfx:print(label, pos.x + 4, pos.y - 4, color)
	end
end

DebugOverlayObjects.drawPath = function(self, layer, path, color)
	if not layer or not layer.gfx or not path or #path == 0 then
		return
	end
	for index = 1, #path do
		local point = path[index]
		if index > 1 then
			local prev = path[index - 1]
			layer.gfx:line(prev.x, prev.y, point.x, point.y, color)
		end
		self:drawMarker(layer, point, color, index == 1 and "p1" or nil)
	end
end

DebugOverlayObjects.drawCircleApprox = function(self, layer, pos, radius, color)
	if not layer or not layer.gfx or not pos or not radius or radius <= 0 then
		return
	end
	layer.gfx:circ(pos.x, pos.y, radius, color)
end

DebugOverlayObjects.drawVisionCone = function(self, layer, object, radius, color)
	if not object or not object.pos or not object.getFacingVector then
		return
	end
	local fx, fy = object:getFacingVector()
	local half_angle = 0.9
	local cos_a = cos(half_angle)
	local sin_a = sin(half_angle)
	local left_x = fx * cos_a - fy * sin_a
	local left_y = fx * sin_a + fy * cos_a
	local right_x = fx * cos_a + fy * sin_a
	local right_y = -fx * sin_a + fy * cos_a
	layer.gfx:line(object.pos.x, object.pos.y, object.pos.x + left_x * radius, object.pos.y + left_y * radius, color)
	layer.gfx:line(object.pos.x, object.pos.y, object.pos.x + right_x * radius, object.pos.y + right_y * radius, color)
	layer.gfx:line(
		object.pos.x + left_x * radius,
		object.pos.y + left_y * radius,
		object.pos.x + right_x * radius,
		object.pos.y + right_y * radius,
		color
	)
end

DebugOverlayObjects.drawObjectGuides = function(self, layer, object)
	if not object or not object.pos then
		return
	end

	if object.autonomy then
		if object.autonomy.path_target then
			self:drawMarker(layer, object.autonomy.path_target, 10, "goal")
		end
		if object.autonomy.path then
			self:drawPath(layer, object.autonomy.path, 9)
		end
	end

	local pathing = self:getObjectPathing(object)
	if pathing then
		if pathing.path_goal then
			self:drawMarker(layer, pathing.path_goal, 10, "goal")
		end
		if pathing.path then
			self:drawPath(layer, pathing.path, 9)
		end
	end

	if object.actions and object.actions.data and object.actions.data.target and object.actions.data.target.pos then
		local target = object.actions.data.target.pos
		layer.gfx:line(object.pos.x, object.pos.y, target.x, target.y, 8)
		self:drawMarker(layer, target, 8, "target")
	end

	if object.perception then
		if object.vision_range and object.vision_range > 0 then
			self:drawVisionCone(layer, object, object.vision_range, 13)
		end
		if object.hearing_range and object.hearing_range > 0 then
			self:drawCircleApprox(layer, object.pos, object.hearing_range, 6)
		end
		if object.perception.seen_target and object.perception.seen_target.pos then
			layer.gfx:line(object.pos.x, object.pos.y, object.perception.seen_target.pos.x, object.perception.seen_target.pos.y, 12)
			self:drawMarker(layer, object.perception.seen_target.pos, 12, "see")
		end
		if object.perception.heard_sound and object.perception.heard_sound.pos then
			self:drawMarker(layer, object.perception.heard_sound.pos, 6, "hear")
		end
	end

	if object.flight_locomotion then
		if object.flight_locomotion.explore_target then
			self:drawMarker(layer, object.flight_locomotion.explore_target, 11, "explore")
		end
		if object.flight_locomotion.landing_target then
			layer.gfx:line(
				object.pos.x,
				object.pos.y,
				object.flight_locomotion.landing_target.x,
				object.flight_locomotion.landing_target.y,
				10
			)
			self:drawMarker(layer, object.flight_locomotion.landing_target, 10, "land")
		end
	end
end

DebugOverlayObjects.drawLayerGuides = function(self, layer)
	for _, object in pairs(layer.entities) do
		if object.debug then
			self:drawObjectGuides(layer, object)
		end
	end
end

DebugOverlayObjects.collectLayerLines = function(self, layer)
	local lines = {}
	self:appendLine(lines, "players:")

	for _, ent in pairs(layer.entities) do
		if ent._type == "Player" then
			local line = self:getObjectLine(ent)
			if line then
				self:appendLine(lines, line)
			end
		end
	end

	return lines
end

return DebugOverlayObjects
