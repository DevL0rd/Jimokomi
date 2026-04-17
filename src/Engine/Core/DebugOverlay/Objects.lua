local DebugOverlayObjects = {}

local function is_glider(object)
	if not object or not object._type then
		return false
	end
	return object._type == "Glider" or sub(object._type, -8) == ":Glider"
end

local function round_debug_value(value)
	return flr(value + (value >= 0 and 0.5 or -0.5))
end

local function append_cached_label(gfx, commands, text, x, y, color)
	if not gfx or not text or text == "" then
		return
	end
	local key = "debug_label:" .. text .. ":" .. tostr(color)
	local entry = gfx:ensureCachedRender(key, function(target)
		target:print(text, 0, 0, color)
	end, {
		w = #text * 4 + 2,
		h = 6,
	})
	if entry then
		gfx.appendOffsetCommands(commands, entry.commands, x, y)
	end
end

local function append_cached_marker(gfx, commands, sx, sy, color)
	local entry = gfx:ensureCachedRender("debug_marker:" .. tostr(color), function(target)
		target:circ(2, 2, 2, color)
		target:circfill(2, 2, 1, color)
	end, {
		w = 4,
		h = 4,
	})
	if entry then
		gfx.appendOffsetCommands(commands, entry.commands, sx - 2, sy - 2)
	end
end

local function append_cached_circle(gfx, commands, sx, sy, radius, color)
	local ir = flr(radius)
	local entry = gfx:ensureCachedRender("debug_circle:" .. ir .. ":" .. tostr(color), function(target)
		target:circ(ir, ir, ir, color)
	end, {
		w = ir * 2,
		h = ir * 2,
	})
	if entry then
		gfx.appendOffsetCommands(commands, entry.commands, sx - ir, sy - ir)
	end
end

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
	if not is_glider(object) then
		return nil
	end
	local name = object.display_name or object._type or "?"
	local mode = object.control_mode == "player" and "ply" or "auto"
	local line = name .. " " .. mode
	if object.health ~= nil and object.max_health ~= nil then
		line ..= " " .. object.health .. "/" .. object.max_health
	end
	if object.actions and object.actions.action then
		line ..= " " .. sub(tostr(object.actions.action), 1, 4)
	end
	if object.ai and object.ai.state then
		line ..= "/" .. sub(tostr(object.ai.state), 1, 4)
	end
	if object.perception then
		if object.perception.seen_target then
			line ..= " s:" .. sub(tostr(object.perception.seen_target.display_name or object.perception.seen_target._type or "?"), 1, 4)
		end
		if object.perception.heard_target then
			line ..= " h:" .. sub(tostr(object.perception.heard_target.display_name or object.perception.heard_target._type or "?"), 1, 4)
		end
	end
	local pathing = self:getObjectPathing(object)
	if pathing then
		local path_len = pathing.path and #pathing.path or 0
		line ..= " p" .. path_len
	end
	return line
end

DebugOverlayObjects.appendMarker = function(self, layer, commands, pos, color, label)
	if not layer or not layer.gfx or not commands or not pos then
		return
	end
	local gfx = layer.gfx
	local sx, sy = layer.camera:layerToScreenXY(pos.x, pos.y)
	append_cached_marker(gfx, commands, sx, sy, color)
	if label and label ~= "" then
		append_cached_label(gfx, commands, label, sx + 4, sy - 4, color)
	end
end

DebugOverlayObjects.appendPath = function(self, layer, commands, path, color)
	if not layer or not layer.gfx or not commands or not path or #path == 0 then
		return
	end
	for index = 1, #path do
		local point = path[index]
		if index > 1 then
			local prev = path[index - 1]
			local sx1, sy1 = layer.camera:layerToScreenXY(prev.x, prev.y)
			local sx2, sy2 = layer.camera:layerToScreenXY(point.x, point.y)
			add(commands, {
				kind = "line",
				x1 = sx1,
				y1 = sy1,
				x2 = sx2,
				y2 = sy2,
				color = color,
			})
		end
		self:appendMarker(layer, commands, point, color, index == 1 and "p1" or nil)
	end
end

DebugOverlayObjects.appendCircleApprox = function(self, layer, commands, pos, radius, color)
	if not layer or not layer.gfx or not commands or not pos or not radius or radius <= 0 then
		return
	end
	local gfx = layer.gfx
	local sx, sy = layer.camera:layerToScreenXY(pos.x, pos.y)
	append_cached_circle(gfx, commands, sx, sy, radius, color)
end

DebugOverlayObjects.appendVisionCone = function(self, layer, commands, object, radius, color)
	if not layer or not layer.gfx or not commands or not object or not object.pos or not object.getFacingVector then
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
	local left_px = round_debug_value(left_x * radius)
	local left_py = round_debug_value(left_y * radius)
	local right_px = round_debug_value(right_x * radius)
	local right_py = round_debug_value(right_y * radius)
	local min_x = min(0, min(left_px, right_px))
	local min_y = min(0, min(left_py, right_py))
	local max_x = max(0, max(left_px, right_px))
	local max_y = max(0, max(left_py, right_py))
	local sx, sy = layer.camera:layerToScreenXY(object.pos.x, object.pos.y)
	local key = "debug_vision:" ..
		left_px .. ":" .. left_py .. ":" .. right_px .. ":" .. right_py .. ":" .. tostr(color)
	local entry = layer.gfx:ensureCachedRender(key, function(target)
		target:line(0, 0, left_px, left_py, color)
		target:line(0, 0, right_px, right_py, color)
		target:line(left_px, left_py, right_px, right_py, color)
	end, {
		w = max_x - min_x + 1,
		h = max_y - min_y + 1,
	})
	if entry then
		layer.gfx.appendOffsetCommands(commands, entry.commands, sx, sy)
	end
end

DebugOverlayObjects.appendObjectGuides = function(self, layer, commands, object)
	if not object or not object.pos then
		return
	end

	if object.autonomy then
		if object.autonomy.path_target then
			self:appendMarker(layer, commands, object.autonomy.path_target, 10, "goal")
		end
		if object.autonomy.path then
			self:appendPath(layer, commands, object.autonomy.path, 9)
		end
	end

	local pathing = self:getObjectPathing(object)
	if pathing then
		if pathing.path_goal then
			self:appendMarker(layer, commands, pathing.path_goal, 10, "goal")
		end
		if pathing.path then
			self:appendPath(layer, commands, pathing.path, 9)
		end
	end

	if object.actions and object.actions.data and object.actions.data.target and object.actions.data.target.pos then
		local target = object.actions.data.target.pos
		local sx1, sy1 = layer.camera:layerToScreenXY(object.pos.x, object.pos.y)
		local sx2, sy2 = layer.camera:layerToScreenXY(target.x, target.y)
		add(commands, {
			kind = "line",
			x1 = sx1,
			y1 = sy1,
			x2 = sx2,
			y2 = sy2,
			color = 8,
		})
		self:appendMarker(layer, commands, target, 8, "target")
	end

	if object.perception then
		if object.vision_range and object.vision_range > 0 then
			self:appendVisionCone(layer, commands, object, object.vision_range, 13)
		end
		if object.hearing_range and object.hearing_range > 0 then
			self:appendCircleApprox(layer, commands, object.pos, object.hearing_range, 6)
		end
		if object.perception.seen_target and object.perception.seen_target.pos then
			local sx1, sy1 = layer.camera:layerToScreenXY(object.pos.x, object.pos.y)
			local sx2, sy2 = layer.camera:layerToScreenXY(object.perception.seen_target.pos.x, object.perception.seen_target.pos.y)
			add(commands, {
				kind = "line",
				x1 = sx1,
				y1 = sy1,
				x2 = sx2,
				y2 = sy2,
				color = 12,
			})
			self:appendMarker(layer, commands, object.perception.seen_target.pos, 12, "see")
		end
		if object.perception.heard_sound and object.perception.heard_sound.pos then
			self:appendMarker(layer, commands, object.perception.heard_sound.pos, 6, "hear")
		end
	end

	if object.flight_locomotion then
		if object.flight_locomotion.explore_target then
			self:appendMarker(layer, commands, object.flight_locomotion.explore_target, 11, "explore")
		end
		if object.flight_locomotion.landing_target then
			local sx1, sy1 = layer.camera:layerToScreenXY(object.pos.x, object.pos.y)
			local sx2, sy2 = layer.camera:layerToScreenXY(object.flight_locomotion.landing_target.x, object.flight_locomotion.landing_target.y)
			add(commands, {
				kind = "line",
				x1 = sx1,
				y1 = sy1,
				x2 = sx2,
				y2 = sy2,
				color = 10,
			})
			self:appendMarker(layer, commands, object.flight_locomotion.landing_target, 10, "land")
		end
	end
end

DebugOverlayObjects.drawLayerGuides = function(self, layer)
	local commands = {}
	for _, object in pairs(layer.entities) do
		if object.debug and object.pos and layer.camera:isPointVisible(object.pos.x, object.pos.y) then
			self:appendObjectGuides(layer, commands, object)
		end
	end
	if #commands > 0 then
		layer.gfx:replayCommandList(commands, 0, 0)
	end
end

DebugOverlayObjects.collectLayerLines = function(self, layer)
	local lines = {}
	for _, ent in pairs(layer.entities) do
		if is_glider(ent) then
			local line = self:getObjectLine(ent)
			if line then
				self:appendLine(lines, line)
			end
		end
	end

	return lines
end

return DebugOverlayObjects
