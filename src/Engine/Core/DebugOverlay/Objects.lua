local Screen = include("src/Engine/Core/Screen.lua")

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

local function should_cache_debug_text(gfx)
	return gfx and gfx.cache_renders and gfx.debug_text_cache_enabled ~= false
end

local function should_cache_debug_shapes(gfx)
	return gfx and gfx.cache_renders and gfx.debug_shape_cache_enabled == true
end

local function observe_command_count(stats, amount)
	if stats then
		stats.commands = (stats.commands or 0) + (amount or 1)
	end
end

local function emit_print(target, stats, text, x, y, color)
	target:print(text, x, y, color)
	observe_command_count(stats, 1)
end

local function emit_line(target, stats, x1, y1, x2, y2, color)
	target:line(x1, y1, x2, y2, color)
	observe_command_count(stats, 1)
end

local function emit_circle_marker(target, stats, sx, sy, color)
	target:circ(sx, sy, 2, color)
	target:circfill(sx, sy, 1, color)
	observe_command_count(stats, 2)
end

local function emit_entry_surface(target, stats, entry, x, y)
	if not target or not entry or not entry.surface then
		return
	end
	target:spr(entry.surface, x or 0, y or 0)
	observe_command_count(stats, 1)
end

local function get_frame_surface_entry(gfx, stats, key, builder, options)
	if not gfx or not key then
		return nil
	end
	local frame_entries = stats and stats.surface_entries or nil
	if frame_entries and frame_entries[key] ~= nil then
		return frame_entries[key]
	end
	local entry = gfx:ensureCachedSurface(key, builder, options)
	if frame_entries then
		frame_entries[key] = entry or false
	end
	return entry
end

local append_cached_label
local append_cached_marker
local append_cached_circle

local function start_scope(profiler, name)
	return profiler and profiler:start(name) or nil
end

local function stop_scope(profiler, scope)
	if profiler and scope then
		profiler:stop(scope)
	end
end

local function object_has_guide_content(self, object)
	if not object or not object.pos then
		return nil
	end
	local autonomy = object.autonomy
	local pathing = self:getObjectPathing(object)
	local actions = object.actions
	local action_target = actions and actions.data and actions.data.target or nil
	local perception = object.perception
	local seen_target = perception and perception.seen_target or nil
	local heard_sound = perception and perception.heard_sound or nil
	local flight = object.flight_locomotion
	local state = {
		autonomy = autonomy,
		pathing = pathing,
		action_target = action_target and action_target.pos and action_target or nil,
		perception = perception,
		seen_target = seen_target and seen_target.pos and seen_target or nil,
		heard_sound = heard_sound and heard_sound.pos and heard_sound or nil,
		flight = flight,
		vision_range = object.vision_range or 0,
		hearing_range = object.hearing_range or 0,
	}
	if autonomy and (autonomy.path_target or autonomy.path) then
		return state
	end
	if pathing and (pathing.path_goal or pathing.path) then
		return state
	end
	if state.action_target then
		return state
	end
	if perception and (state.vision_range > 0 or state.hearing_range > 0 or state.seen_target or state.heard_sound) then
		return state
	end
	if flight and (flight.explore_target or flight.landing_target) then
		return state
	end
	return nil
end

local function append_world_marker(gfx, target, stats, pos, color, label)
	if not gfx or not target or not pos then
		return
	end
	local camera = gfx.camera
	if camera and not camera:isPointVisible(pos.x, pos.y) then
		return
	end
	emit_circle_marker(target, stats, pos.x, pos.y, color)
end

local function append_world_circle(gfx, target, stats, pos, radius, color)
	if not gfx or not target or not pos or not radius or radius <= 0 then
		return
	end
	target:circ(pos.x, pos.y, flr(radius), color)
	observe_command_count(stats, 1)
end

local function draw_screen_perception_guides(layer, profiler, object)
	return
end

local function append_world_vision_cone(layer, target, stats, object, radius, color)
	if not layer or not layer.gfx or not target or not object or not object.pos or not object.getFacingVector then
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
	emit_line(target, stats, object.pos.x, object.pos.y, object.pos.x + left_px, object.pos.y + left_py, color)
	emit_line(target, stats, object.pos.x, object.pos.y, object.pos.x + right_px, object.pos.y + right_py, color)
	emit_line(
		target,
		stats,
		object.pos.x + left_px,
		object.pos.y + left_py,
		object.pos.x + right_px,
		object.pos.y + right_py,
		color
	)
end

local function append_world_path(self, layer, target, stats, path, color)
	if not layer or not layer.gfx or not target or not path or #path == 0 then
		return
	end
	local profiler = stats and stats.profiler or nil
	local path_len = #path
	local polyline_step = 1
	if path_len > 24 then
		polyline_step = 4
	elseif path_len > 14 then
		polyline_step = 3
	elseif path_len > 8 then
		polyline_step = 2
	end
	if #path > 1 then
		local polyline_scope = start_scope(profiler, "engine.debug_overlay.guides.path_polyline")
		local prev = path[1]
		for index = 2, #path, polyline_step do
			local point = path[index]
			emit_line(target, stats, prev.x, prev.y, point.x, point.y, color)
			prev = point
		end
		if prev ~= path[path_len] then
			local point = path[path_len]
			emit_line(target, stats, prev.x, prev.y, point.x, point.y, color)
		end
		stop_scope(profiler, polyline_scope)
	end
	local marker_scope = start_scope(profiler, "engine.debug_overlay.guides.path_markers")
	local marker_step = 1
	if path_len > 24 then
		marker_step = 6
	elseif path_len > 14 then
		marker_step = 4
	elseif path_len > 8 then
		marker_step = 2
	end
	for index = 1, path_len, marker_step do
		append_world_marker(layer.gfx, target, stats, path[index], color, index == 1 and "p1" or nil)
	end
	if path_len > 1 and ((path_len - 1) % marker_step ~= 0) then
		append_world_marker(layer.gfx, target, stats, path[path_len], color, nil)
	end
	stop_scope(profiler, marker_scope)
end

local function count_active_buckets(index)
	if not index or not index.active_buckets then
		return 0
	end
	local total = 0
	for i = 1, #index.active_buckets do
		local bucket = index.active_buckets[i]
		if bucket and bucket.stamp == index.build_stamp and #bucket > 0 then
			total += 1
		end
	end
	return total
end

local function count_visible_buckets(layer, index)
	if not layer or not layer.camera or not index or not index.active_buckets then
		return 0
	end
	local cell_size = index.cell_size or 32
	local total = 0
	for i = 1, #index.active_buckets do
		local bucket = index.active_buckets[i]
		if bucket and bucket.stamp == index.build_stamp and #bucket > 0 then
			local left = (bucket.cell_x or 0) * cell_size
			local top = (bucket.cell_y or 0) * cell_size
			if layer.camera:isRectVisible(left, top, cell_size, cell_size) then
				total += 1
			end
		end
	end
	return total
end

local function draw_spatial_buckets(layer, target, stats, index, color, max_buckets)
	if not layer or not layer.camera or not target or not index or not index.active_buckets then
		return 0
	end
	local cell_size = index.cell_size or 32
	local drawn = 0
	local limit = max_buckets or 64
	for i = 1, #index.active_buckets do
		if drawn >= limit then
			break
		end
		local bucket = index.active_buckets[i]
		if bucket and bucket.stamp == index.build_stamp and #bucket > 0 then
			local left = (bucket.cell_x or 0) * cell_size
			local top = (bucket.cell_y or 0) * cell_size
			if layer.camera:isRectVisible(left, top, cell_size, cell_size) then
				target:rect(left, top, left + cell_size - 1, top + cell_size - 1, color)
				observe_command_count(stats, 1)
				drawn += 1
			end
		end
	end
	return drawn
end

local function append_object_guides_world(self, layer, target, stats, object, guide_state)
	if not object or not object.pos then
		return
	end
	local profiler = stats and stats.profiler or nil
	local autonomy = guide_state and guide_state.autonomy or object.autonomy
	local pathing = guide_state and guide_state.pathing or self:getObjectPathing(object)
	local action_target = guide_state and guide_state.action_target or nil
	local perception = guide_state and guide_state.perception or object.perception
	local seen_target = guide_state and guide_state.seen_target or nil
	local heard_sound = guide_state and guide_state.heard_sound or nil
	local flight = guide_state and guide_state.flight or object.flight_locomotion

	if autonomy then
		local marker_scope = start_scope(profiler, "engine.debug_overlay.guides.markers")
		if autonomy.path_target then
			append_world_marker(layer.gfx, target, stats, autonomy.path_target, 10, "goal")
		end
		stop_scope(profiler, marker_scope)
		if autonomy.path then
			append_world_path(self, layer, target, stats, autonomy.path, 9)
		end
	end

	if pathing then
		local marker_scope = start_scope(profiler, "engine.debug_overlay.guides.markers")
		if pathing.path_goal then
			append_world_marker(layer.gfx, target, stats, pathing.path_goal, 10, "goal")
		end
		stop_scope(profiler, marker_scope)
		if pathing.path then
			append_world_path(self, layer, target, stats, pathing.path, 9)
		end
	end

	if action_target and action_target.pos then
		local link_scope = start_scope(profiler, "engine.debug_overlay.guides.links")
		local target_pos = action_target.pos
		emit_line(target, stats, object.pos.x, object.pos.y, target_pos.x, target_pos.y, 8)
		append_world_marker(layer.gfx, target, stats, target_pos, 8, "target")
		stop_scope(profiler, link_scope)
	end

	if perception then
	end

	if flight then
		local marker_scope = start_scope(profiler, "engine.debug_overlay.guides.markers")
		if flight.explore_target then
			append_world_marker(layer.gfx, target, stats, flight.explore_target, 11, "explore")
		end
		stop_scope(profiler, marker_scope)
		if flight.landing_target then
			local link_scope = start_scope(profiler, "engine.debug_overlay.guides.links")
			local landing_target = flight.landing_target
			emit_line(target, stats, object.pos.x, object.pos.y, landing_target.x, landing_target.y, 10)
			append_world_marker(layer.gfx, target, stats, landing_target, 10, "land")
			stop_scope(profiler, link_scope)
		end
	end
end

append_cached_label = function(gfx, target, stats, text, x, y, color)
	return
end

append_cached_marker = function(gfx, target, stats, sx, sy, color)
	if not should_cache_debug_shapes(gfx) then
		emit_circle_marker(target, stats, sx, sy, color)
		return
	end
	local key = "debug_marker:" .. tostr(color)
	local entry = get_frame_surface_entry(gfx, stats, key, function(surface_target)
		surface_target:circ(2, 2, 2, color)
		surface_target:circfill(2, 2, 1, color)
	end, {
		w = 5,
		h = 5,
		force_cache = true,
		cache_tag = "debug.shapes.marker",
		cache_profile_key = "debug.shapes.marker",
		cache_profile_signature = tostr(color),
		clear_color = 0,
	})
	if entry then
		emit_entry_surface(target, stats, entry, sx - 2, sy - 2)
	end
end

append_cached_circle = function(gfx, target, stats, sx, sy, radius, color)
	local ir = flr(radius)
	if not should_cache_debug_shapes(gfx) then
		target:circ(sx, sy, ir, color)
		observe_command_count(stats, 1)
		return
	end
	local key = "debug_circle:" .. ir .. ":" .. tostr(color)
	local entry = get_frame_surface_entry(gfx, stats, key, function(surface_target)
		surface_target:circ(ir, ir, ir, color)
	end, {
		w = ir * 2 + 1,
		h = ir * 2 + 1,
		force_cache = true,
		cache_tag = "debug.shapes.circle",
		cache_profile_key = "debug.shapes.circle",
		cache_profile_signature = ir .. ":" .. tostr(color),
		clear_color = 0,
	})
	if entry then
		emit_entry_surface(target, stats, entry, sx - ir, sy - ir)
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
	local mode = object.isPlayerControlled and object:isPlayerControlled() and "ply" or "auto"
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

DebugOverlayObjects.appendMarker = function(self, layer, target, stats, pos, color, label)
	if not layer or not layer.gfx or not target or not pos then
		return
	end
	local gfx = layer.gfx
	local sx, sy = layer.camera:layerToScreenXY(pos.x, pos.y)
	append_cached_marker(gfx, target, stats, sx, sy, color)
end

DebugOverlayObjects.appendPath = function(self, layer, target, stats, path, color)
	if not layer or not layer.gfx or not target or not path or #path == 0 then
		return
	end
	local path_len = #path
	local screen_points = {}
	for index = 1, path_len do
		local point = path[index]
		local sx, sy = layer.camera:layerToScreenXY(point.x, point.y)
		screen_points[index] = {
			x = sx,
			y = sy,
		}
	end
	for index = 1, path_len do
		local point = screen_points[index]
		if index > 1 then
			local prev = screen_points[index - 1]
			emit_line(target, stats, prev.x, prev.y, point.x, point.y, color)
		end
		append_cached_marker(layer.gfx, target, stats, point.x, point.y, color)
	end
end

DebugOverlayObjects.appendCircleApprox = function(self, layer, target, stats, pos, radius, color)
	if not layer or not layer.gfx or not target or not pos or not radius or radius <= 0 then
		return
	end
	local gfx = layer.gfx
	local sx, sy = layer.camera:layerToScreenXY(pos.x, pos.y)
	append_cached_circle(gfx, target, stats, sx, sy, radius, color)
end

DebugOverlayObjects.appendVisionCone = function(self, layer, target, stats, object, radius, color)
	return
end

DebugOverlayObjects.appendObjectGuides = function(self, layer, target, stats, object)
	if not object or not object.pos then
		return
	end

	if object.autonomy then
		if object.autonomy.path_target then
			self:appendMarker(layer, target, stats, object.autonomy.path_target, 10, "goal")
		end
		if object.autonomy.path then
			self:appendPath(layer, target, stats, object.autonomy.path, 9)
		end
	end

	local pathing = self:getObjectPathing(object)
	if pathing then
		if pathing.path_goal then
			self:appendMarker(layer, target, stats, pathing.path_goal, 10, "goal")
		end
		if pathing.path then
			self:appendPath(layer, target, stats, pathing.path, 9)
		end
	end

	if object.actions and object.actions.data and object.actions.data.target and object.actions.data.target.pos then
		local target_pos = object.actions.data.target.pos
		local sx1, sy1 = layer.camera:layerToScreenXY(object.pos.x, object.pos.y)
		local sx2, sy2 = layer.camera:layerToScreenXY(target_pos.x, target_pos.y)
		emit_line(target, stats, sx1, sy1, sx2, sy2, 8)
		self:appendMarker(layer, target, stats, target_pos, 8, "target")
	end

	if object.perception then
	end

	if object.flight_locomotion then
		if object.flight_locomotion.explore_target then
			self:appendMarker(layer, target, stats, object.flight_locomotion.explore_target, 11, "explore")
		end
		if object.flight_locomotion.landing_target then
			local sx1, sy1 = layer.camera:layerToScreenXY(object.pos.x, object.pos.y)
			local sx2, sy2 = layer.camera:layerToScreenXY(object.flight_locomotion.landing_target.x, object.flight_locomotion.landing_target.y)
			emit_line(target, stats, sx1, sy1, sx2, sy2, 10)
			self:appendMarker(layer, target, stats, object.flight_locomotion.landing_target, 10, "land")
		end
	end
end

DebugOverlayObjects.drawLayerGuides = function(self, layer)
	local profiler = layer and layer.engine and layer.engine.profiler or nil
	local collect_scope = start_scope(profiler, "engine.debug_overlay.guides.collect")
	local guide_objects = {}
	local guide_candidates = layer.drawable_entities or layer.entities or {}
	local max_guides = 6
	for i = 1, #guide_candidates do
		local object = guide_candidates[i]
		if object.debug and object.pos and layer.camera:isPointVisible(object.pos.x, object.pos.y) then
			local guide_state = object_has_guide_content(self, object)
			if guide_state then
				add(guide_objects, {
					object = object,
					state = guide_state,
				})
				if #guide_objects >= max_guides then
					break
				end
			end
		end
	end
	stop_scope(profiler, collect_scope)

	local last_command_count = 0
	if #guide_objects > 0 then
		local render_scope = start_scope(profiler, "engine.debug_overlay.guides.render")
		local origin_x, origin_y = layer.camera:layerToScreenXY(0, 0)
		local target = layer.gfx:buildImmediateTarget(origin_x, origin_y)
		for i = 1, #guide_objects do
			local guide_info = guide_objects[i]
			local object = guide_info.object
			local assembly_scope = start_scope(profiler, "engine.debug_overlay.guides.command_assembly")
			local stats = {
				commands = 0,
				profiler = profiler,
			}
			append_object_guides_world(self, layer, target, stats, object, guide_info.state)
			last_command_count += stats.commands or 0
			stop_scope(profiler, assembly_scope)
			draw_screen_perception_guides(layer, profiler, object)
		end
		stop_scope(profiler, render_scope)
	end
	if profiler then
		profiler:observe("engine.debug_overlay.guide_objects", #guide_objects)
		profiler:observe("engine.debug_overlay.guide_commands", last_command_count)
		profiler:observe("engine.debug_overlay.entity_dynamic_chunks", count_active_buckets(layer.entity_spatial_index))
		profiler:observe("engine.debug_overlay.entity_static_chunks", count_active_buckets(layer.entity_static_spatial_index))
		profiler:observe("engine.debug_overlay.collidable_dynamic_chunks", count_active_buckets(layer.collidable_spatial_index))
		profiler:observe("engine.debug_overlay.collidable_static_chunks", count_active_buckets(layer.collidable_static_spatial_index))
		profiler:observe("engine.debug_overlay.entity_dynamic_chunks_visible", count_visible_buckets(layer, layer.entity_spatial_index))
		profiler:observe("engine.debug_overlay.entity_static_chunks_visible", count_visible_buckets(layer, layer.entity_static_spatial_index))
		profiler:observe("engine.debug_overlay.collidable_dynamic_chunks_visible", count_visible_buckets(layer, layer.collidable_spatial_index))
		profiler:observe("engine.debug_overlay.collidable_static_chunks_visible", count_visible_buckets(layer, layer.collidable_static_spatial_index))
	end
end

DebugOverlayObjects.collectLayerLines = function(self, layer)
	local lines = {}
	local entity_dynamic = layer and layer.entity_spatial_index and layer.entity_spatial_index.entity_count or 0
	local entity_static = layer and layer.entity_static_spatial_index and layer.entity_static_spatial_index.entity_count or 0
	local collidable_dynamic = layer and layer.collidable_spatial_index and layer.collidable_spatial_index.entity_count or 0
	local collidable_static = layer and layer.collidable_static_spatial_index and layer.collidable_static_spatial_index.entity_count or 0
	local entity_dynamic_chunks = count_active_buckets(layer and layer.entity_spatial_index)
	local entity_static_chunks = count_active_buckets(layer and layer.entity_static_spatial_index)
	local collidable_dynamic_chunks = count_active_buckets(layer and layer.collidable_spatial_index)
	local collidable_static_chunks = count_active_buckets(layer and layer.collidable_static_spatial_index)
	local collision_dynamic_chunks = count_active_buckets(layer and layer.collision_dynamic_spatial_index)
	local collision_static_chunks = count_active_buckets(layer and layer.collision_static_spatial_index)
	local entity_visible_chunks = count_visible_buckets(layer, layer and layer.entity_spatial_index)
	local entity_static_visible_chunks = count_visible_buckets(layer, layer and layer.entity_static_spatial_index)
	local collidable_visible_chunks = count_visible_buckets(layer, layer and layer.collidable_spatial_index)
	local collidable_static_visible_chunks = count_visible_buckets(layer, layer and layer.collidable_static_spatial_index)
	local spatial_cell_size = layer and layer.spatial_index_cell_size or 32
	local collision_cell_size = layer and layer.collision_broadphase_cell_size or spatial_cell_size
	local stress_ball_count = 0

	for _, ent in pairs(layer.entities) do
		if ent and ent.stress_test_ball == true and not ent._delete then
			stress_ball_count += 1
		end
	end

	add(lines, {
		text = "Spatial cell " .. spatial_cell_size .. "  Collision cell " .. collision_cell_size,
		color = 6,
	})
	add(lines, {
		text = "Collision buckets moving " .. collision_dynamic_chunks .. "  static " .. collision_static_chunks,
		color = 6,
	})
	add(lines, {
		text = "Moving entities: " .. entity_dynamic ..
			"  Chunks: " .. entity_dynamic_chunks ..
			"  On screen: " .. entity_visible_chunks,
		color = 7,
		chip_color = 3,
	})
	add(lines, {
		text = "Static entities: " .. entity_static ..
			"  Chunks: " .. entity_static_chunks ..
			"  On screen: " .. entity_static_visible_chunks,
		color = 7,
		chip_color = 11,
	})
	add(lines, {
		text = "Moving collidables: " .. collidable_dynamic ..
			"  Chunks: " .. collidable_dynamic_chunks ..
			"  On screen: " .. collidable_visible_chunks,
		color = 7,
		chip_color = 9,
	})
	add(lines, {
		text = "Static collidables: " .. collidable_static ..
			"  Chunks: " .. collidable_static_chunks ..
			"  On screen: " .. collidable_static_visible_chunks,
		color = 7,
		chip_color = 10,
	})
	if stress_ball_count > 0 then
		add(lines, {
			text = "Stress test balls: " .. stress_ball_count,
			color = 7,
			chip_color = 13,
		})
	end
	add(lines, {
		text = "Objects",
		color = 10,
	})

	for _, ent in pairs(layer.entities) do
		if is_glider(ent) then
			local line = self:getObjectLine(ent)
			if line then
				add(lines, {
					text = line,
					color = 7,
				})
			end
		end
	end

	return lines
end

return DebugOverlayObjects
