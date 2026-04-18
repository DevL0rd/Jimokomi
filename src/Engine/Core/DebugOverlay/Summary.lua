local Screen = include("src/Engine/Core/Screen.lua")

local DebugOverlaySummary = {}

local SUMMARY_PANEL_STYLE = {
	key = "debug_summary_panel",
	shadow_offset = 1,
	shadow_color = 1,
	bg_top = 1,
	bg_bottom = 2,
	border_color = 6,
	inner_border_color = 5,
	highlight_color = 7,
	speckle_density = 0,
}

local function get_overlay_gfx(engine)
	for _, layer in pairs(engine.layers) do
		if layer and layer.gfx then
			return layer.gfx
		end
	end
	return nil
end

local function get_scope_entry(profiler, scope_name, entry_index)
	if not profiler or not profiler.last_report_entries then
		return nil
	end
	if entry_index then
		return entry_index[scope_name]
	end
	for i = 1, #profiler.last_report_entries do
		local entry = profiler.last_report_entries[i]
		if entry.name == scope_name then
			return entry
		end
	end
	return nil
end

local function build_entry_index(profiler)
	if not profiler or not profiler.last_report_entries then
		return nil
	end
	local index = {}
	for i = 1, #profiler.last_report_entries do
		local entry = profiler.last_report_entries[i]
		index[entry.name] = entry
	end
	return index
end

local function get_counter_value(profiler, scope_name, entry_index)
	local entry = get_scope_entry(profiler, scope_name, entry_index)
	if not entry then
		return 0
	end
	if entry.counter and entry.counter > 0 then
		return entry.counter
	end
	if entry.observed_avg and entry.observed_avg > 0 then
		return entry.observed_avg
	end
	if entry.hz and entry.hz > 0 then
		return entry.hz
	end
	return 0
end

local function round_value(value)
	return flr((value or 0) * 10 + 0.5) / 10
end

local function add_line(lines, text, color, chip_color)
	add(lines, {
		text = text or "",
		color = color or 7,
		chip_color = chip_color,
	})
end

local function add_blank(lines)
	add_line(lines, "", 7, nil)
end

local function normalize_line_entry(line)
	if type(line) == "table" then
		return {
			text = line.text or "",
			color = line.color or 7,
			chip_color = line.chip_color,
		}
	end
	return {
		text = line or "",
		color = 7,
		chip_color = nil,
	}
end

local function measure_lines(lines)
	local max_width = 0
	for i = 1, #lines do
		local entry = normalize_line_entry(lines[i])
		local chip_width = entry.chip_color and 8 or 0
		local line_width = chip_width + (#entry.text * 4)
		if line_width > max_width then
			max_width = line_width
		end
	end
	return max_width
end

local function build_summary_lines(self, engine)
	local profiler = engine and engine.profiler or nil
	local report_serial = profiler and profiler.report_serial or 0
	local now = time()
	local refresh_ms = self and self.summary_refresh_ms or 250
	if self._cached_summary_lines and self._cached_summary_lines_until and now < self._cached_summary_lines_until and
		self._cached_summary_report_serial == report_serial then
		return self._cached_summary_lines
	end

	local lines = {}
	profiler = engine.profiler
	local entry_index = build_entry_index(profiler)
	local runtime = profiler and profiler.runtime_stats or nil
	local draw_hz = get_scope_entry(profiler, "engine.draw", entry_index)
	local update_hz = get_scope_entry(profiler, "engine.update", entry_index)

	local cpu = runtime and runtime.cpu or 0
	local fps = runtime and runtime.fps or 0
	local draw_rate = draw_hz and draw_hz.hz or (runtime and runtime.draw_fps) or 0
	local update_rate = update_hz and update_hz.hz or 0
	add_line(lines, "Runtime", 10)
	add_line(
		lines,
		"Frame rate " .. flr(fps + 0.5) ..
			"  Draw " .. flr(draw_rate + 0.5) ..
			"  Update " .. flr(update_rate + 0.5) ..
			"  CPU " .. round_value(cpu),
		7
	)

	local cpu_max = runtime and runtime.cpu_max or 0
	local dt_max = runtime and runtime.frame_dt_ms_max or 0
	local over_budget = runtime and runtime.cpu_spikes_over_10 or 0
	add_line(lines, "Peak CPU " .. round_value(cpu_max) .. "  Max frame ms " .. round_value(dt_max) .. "  Over budget " .. over_budget, 6)
	add_blank(lines)

	local raycasts = get_scope_entry(profiler, "world.raycast.cast", entry_index)
	local sounds = get_scope_entry(profiler, "game.agent.update_sounding", entry_index)
	local los = get_scope_entry(profiler, "world.senses.line_of_sight", entry_index)
	add_line(lines, "Queries", 10)
	add_line(
		lines,
		"Raycasts " .. flr((raycasts and raycasts.hz or 0) + 0.5) ..
			"  Sound checks " .. flr((sounds and sounds.hz or 0) + 0.5) ..
			"  Sight checks " .. flr((los and los.hz or 0) + 0.5),
		7
	)

	local contacts = flr(get_counter_value(profiler, "collision.resolver.contacts_processed", entry_index) + 0.5)
	local circle_checks = flr(get_counter_value(profiler, "collision.tiles.circle_checks", entry_index) + 0.5)
	add_line(lines, "Collision contacts " .. contacts .. "  Tile checks " .. circle_checks, 7)

	local cache_hits = flr(get_counter_value(profiler, "render.cache.hits", entry_index) + 0.5)
	local cache_rebuilds = flr(get_counter_value(profiler, "render.cache.rebuilds", entry_index) + 0.5)
	local retained_hits = flr(get_counter_value(profiler, "render.retained.hits", entry_index) + 0.5)
	local retained_rebuilds = flr(get_counter_value(profiler, "render.retained.rebuilds", entry_index) + 0.5)
	local total_cache = cache_hits + cache_rebuilds
	local total_retained = retained_hits + retained_rebuilds
	local cache_pct = total_cache > 0 and flr((cache_hits / total_cache) * 100 + 0.5) or 0
	local retained_pct = total_retained > 0 and flr((retained_hits / total_retained) * 100 + 0.5) or 0
	add_line(lines, "Surface cache hit rate " .. cache_pct .. "%  Retained hit rate " .. retained_pct .. "%", 7)
	add_blank(lines)
	add_line(lines, "Spatial Legend", 10)
	add_line(lines, "dyn entity", 7, 3)
	add_line(lines, "static entity", 7, 11)
	add_line(lines, "dyn collidable", 7, 9)
	add_line(lines, "static collidable", 7, 10)
	add_blank(lines)
	add_line(lines, "Objects", 10)

	for _, layer in pairs(engine.layers) do
		local layer_lines = self:collectLayerLines(layer)
		for _, line in pairs(layer_lines) do
			add(lines, normalize_line_entry(line))
		end
	end

	self._cached_summary_lines = lines
	self._cached_summary_report_serial = report_serial
	self._cached_summary_lines_until = now + (refresh_ms / 1000)
	self._cached_summary_draw_serial = (self._cached_summary_draw_serial or 0) + 1
	return lines
end

local function clamp01(value)
	return mid(0, value or 0, 1)
end

local function draw_meter(target, x, y, w, h, fill_ratio, color, back_color, label)
	target:rectfill(x, y, x + w - 1, y + h - 1, back_color or 1)
	target:rect(x, y, x + w - 1, y + h - 1, 5)
	local fill_w = flr((w - 2) * clamp01(fill_ratio) + 0.5)
	if fill_w > 0 then
		target:rectfill(x + 1, y + 1, x + fill_w, y + h - 2, color)
	end
	if label and label ~= "" then
		local inner_w = max(1, w - 6)
		local text_x = x + 3 + flr((inner_w - (#label * 4)) * 0.5)
		local text_y = y + flr((h - 5) * 0.5)
		target:print(label, text_x, text_y, 7)
	end
end

local function draw_chip_row(target, x, y, colors)
	for i = 1, #colors do
		local chip_x = x + (i - 1) * 6
		target:rectfill(chip_x, y, chip_x + 3, y + 3, colors[i])
		target:rect(chip_x, y, chip_x + 3, y + 3, 1)
	end
end

local function draw_visual_summary(self, engine, gfx)
	local profiler = engine and engine.profiler or nil
	local runtime = profiler and profiler.runtime_stats or nil
	local entry_index = build_entry_index(profiler)
	local draw_entry = get_scope_entry(profiler, "engine.draw", entry_index)
	local update_entry = get_scope_entry(profiler, "engine.update", entry_index)
	local entities_entry = get_scope_entry(profiler, "layer.render.entities", entry_index)
	local map_entry = get_scope_entry(profiler, "layer.render.map", entry_index)
	local overlay_entry = get_scope_entry(profiler, "engine.debug_overlay.draw", entry_index)
	local collisions_entry = get_scope_entry(profiler, "layer.sim.resolve_collisions", entry_index)
	local entity_update_entry = get_scope_entry(profiler, "layer.sim.update_entities", entry_index)
	local cache_hits = flr(get_counter_value(profiler, "render.cache.hits", entry_index) + 0.5)
	local cache_rebuilds = flr(get_counter_value(profiler, "render.cache.rebuilds", entry_index) + 0.5)
	local cache_total = max(1, cache_hits + cache_rebuilds)
	local cache_ratio = cache_hits / cache_total
	local fps = runtime and runtime.fps or 0
	local frame_ms = runtime and runtime.frame_dt_ms_avg or 0
	local draw_cpu_now = engine and engine.last_draw_cpu_frame or 0
	local update_cpu_now = engine and engine.last_update_cpu_frame or 0
	local total_frame_cpu = max(0.0001, draw_cpu_now + update_cpu_now, runtime and runtime.cpu or 0)
	local draw_ms_now = frame_ms * (draw_cpu_now / total_frame_cpu)
	local update_ms_now = frame_ms * (update_cpu_now / total_frame_cpu)
	local draw_parts = max(1, (entities_entry and entities_entry.total_cpu or 0) + (map_entry and map_entry.total_cpu or 0) + (overlay_entry and overlay_entry.total_cpu or 0))
	local update_parts = max(1, (collisions_entry and collisions_entry.total_cpu or 0) + (entity_update_entry and entity_update_entry.total_cpu or 0))
	local width = 102
	local height = 48
	local draw_x = Screen.w - width - 4
	local draw_y = 4
	local summary_scope = profiler and profiler:start("engine.debug_overlay.summary_panel") or nil
	local panel = gfx:getCachedPanel(width, height, SUMMARY_PANEL_STYLE, {
		cache_tag = "debug.summary.panel",
	})
	gfx:drawPanel(panel, draw_x, draw_y)
	local target = gfx:buildImmediateTarget(draw_x, draw_y)
	target:print("F", 7, 5, 7)
	draw_meter(
		target,
		15,
		5,
		80,
		8,
		fps / 60,
		fps >= 60 and 11 or (fps >= 30 and 10 or 8),
		1,
		tostr(flr(fps + 0.5)) .. "fps"
	)
	target:print("MS", 7, 17, 7)
	draw_meter(
		target,
		15,
		17,
		80,
		8,
		frame_ms / 33.3,
		frame_ms < 20 and 11 or (frame_ms < 33.3 and 10 or 8),
		1,
		tostr(flr(frame_ms + 0.5)) .. "ms"
	)
	target:print("D", 7, 29, 12)
	target:print("U", 53, 29, 9)
	draw_meter(
		target,
		15,
		29,
		34,
		8,
		draw_ms_now / max(1, draw_ms_now + update_ms_now),
		12,
		1,
		tostr(flr(draw_ms_now + 0.5)) .. "ms"
	)
	draw_meter(
		target,
		57,
		29,
		34,
		8,
		update_ms_now / max(1, draw_ms_now + update_ms_now),
		9,
		1,
		tostr(flr(update_ms_now + 0.5)) .. "ms"
	)
	if profiler then
		profiler:stop(summary_scope)
	end
end

DebugOverlaySummary.draw = function(self, engine)
	local gfx = get_overlay_gfx(engine)
	local profiler = engine and engine.profiler or nil
	local guides_scope = profiler and profiler:start("engine.debug_overlay.guides") or nil
	for _, layer in pairs(engine.layers) do
		if engine.debug_guides_enabled == true and layer.running and self.drawLayerGuides then
			self:drawLayerGuides(layer)
		end
	end
	if profiler then profiler:stop(guides_scope) end
	if gfx and (engine.debug_overlay_enabled == true or engine.performance_panel_enabled == true) then
		draw_visual_summary(self, engine, gfx)
	end
end

return DebugOverlaySummary
