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

local function get_scope_entry(profiler, scope_name)
	if not profiler or not profiler.last_report_entries then
		return nil
	end
	for i = 1, #profiler.last_report_entries do
		local entry = profiler.last_report_entries[i]
		if entry.name == scope_name then
			return entry
		end
	end
	return nil
end

local function get_counter_value(profiler, scope_name)
	local entry = get_scope_entry(profiler, scope_name)
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

local function build_summary_lines(self, engine)
	local lines = {}
	local profiler = engine.profiler
	local runtime = profiler and profiler.runtime_stats or nil
	local draw_hz = get_scope_entry(profiler, "engine.draw")
	local update_hz = get_scope_entry(profiler, "engine.update")

	local cpu = runtime and runtime.cpu or 0
	local fps = runtime and runtime.fps or 0
	local draw_rate = draw_hz and draw_hz.hz or (runtime and runtime.draw_fps) or 0
	local update_rate = update_hz and update_hz.hz or 0
	add(lines, "fps " .. flr(fps + 0.5) .. " dr " .. flr(draw_rate + 0.5) .. " up " .. flr(update_rate + 0.5) .. " cpu " .. round_value(cpu))

	local cpu_max = runtime and runtime.cpu_max or 0
	local dt_max = runtime and runtime.frame_dt_ms_max or 0
	local over_budget = runtime and runtime.cpu_spikes_over_10 or 0
	add(lines, "spk " .. round_value(cpu_max) .. " dt " .. round_value(dt_max) .. " >1 " .. over_budget)

	local raycasts = get_scope_entry(profiler, "world.raycast.cast")
	local sounds = get_scope_entry(profiler, "game.agent.update_sounding")
	local los = get_scope_entry(profiler, "world.senses.line_of_sight")
	add(lines, "ray " .. flr((raycasts and raycasts.hz or 0) + 0.5) ..
		" snd " .. flr((sounds and sounds.hz or 0) + 0.5) ..
		" los " .. flr((los and los.hz or 0) + 0.5))

	local contacts = flr(get_counter_value(profiler, "collision.resolver.contacts_processed") + 0.5)
	local circle_checks = flr(get_counter_value(profiler, "collision.tiles.circle_checks") + 0.5)
	add(lines, "col " .. contacts .. " tile " .. circle_checks)

	local cache_hits = flr(get_counter_value(profiler, "render.cache.hits") + 0.5)
	local cache_rebuilds = flr(get_counter_value(profiler, "render.cache.rebuilds") + 0.5)
	local retained_hits = flr(get_counter_value(profiler, "render.retained.hits") + 0.5)
	local retained_rebuilds = flr(get_counter_value(profiler, "render.retained.rebuilds") + 0.5)
	local total_cache = cache_hits + cache_rebuilds
	local total_retained = retained_hits + retained_rebuilds
	local cache_pct = total_cache > 0 and flr((cache_hits / total_cache) * 100 + 0.5) or 0
	local retained_pct = total_retained > 0 and flr((retained_hits / total_retained) * 100 + 0.5) or 0
	add(lines, "cache " .. cache_pct .. "% ret " .. retained_pct .. "%")

	for _, layer in pairs(engine.layers) do
		local layer_lines = self:collectLayerLines(layer)
		for _, line in pairs(layer_lines) do
			add(lines, line)
		end
	end

	return lines
end

local function get_summary_state(self, gfx)
	self.summary_state = self.summary_state or {}
	local state = self.summary_state
	if state.gfx ~= gfx then
		state.gfx = gfx
		state.root = nil
		state.panel = nil
		state.lines = {}
		state.width = 154
		state.height = 0
	end
	return state
end

local function ensure_summary_tree(self, gfx)
	local state = get_summary_state(self, gfx)
	if state.root then
		return state
	end

	state.root = gfx:newRetainedGroup({
		key = "debug.summary.root",
		cache_mode = "surface",
		w = state.width,
		h = 0,
	})
	state.panel = gfx:newRetainedPanel({
		key = "debug.summary.panel",
		cache_mode = "retained",
		w = state.width,
		h = 0,
		style = SUMMARY_PANEL_STYLE,
	})
	state.root:addChild(state.panel)

	return state
end

local function ensure_line_node(gfx, state, index)
	local node = state.lines[index]
	if node then
		return node
	end
	node = gfx:newRetainedLeaf({
		key = "debug.summary.line." .. index,
		cache_mode = "retained",
		x = 5,
		y = 4 + (index - 1) * 7,
		w = state.width - 10,
		h = 6,
	})
	state.lines[index] = node
	state.root:addChild(node)
	return node
end

local function configure_summary_ui(gfx, state, lines)
	local height = max(12, #lines * 7 + 8)
	state.height = height
	state.root:setBounds(state.width, height)
	state.panel:setBounds(state.width, height)
	state.panel:setStateKey("summary:" .. height)

	for i = 1, #lines do
		local text = lines[i]
		local node = ensure_line_node(gfx, state, i)
		node:setPosition(5, 4 + (i - 1) * 7)
		node:setBounds(state.width - 10, 6)
		if node.state_key ~= text then
			node:setStateKey(text)
			node:setBuilder(function(target)
				target:print(text, 0, 0, 7)
			end)
		end
	end

	for i = #lines + 1, #state.lines do
		local node = state.lines[i]
		if node.state_key ~= "__hidden" then
			node:setStateKey("__hidden")
			node:setBuilder(nil)
		end
	end
end

DebugOverlaySummary.draw = function(self, engine)
	local gfx = get_overlay_gfx(engine)
	local lines = build_summary_lines(self, engine)
	if not gfx then
		self:drawLines(lines, 1, 1, 8)
		return
	end

	for _, layer in pairs(engine.layers) do
		if engine.debug_guides_enabled == true and layer.running and self.drawLayerGuides then
			self:drawLayerGuides(layer)
		end
	end

	local state = ensure_summary_tree(self, gfx)
	configure_summary_ui(gfx, state, lines)
	local draw_x = Screen.w - state.width - 4
	state.root:draw(draw_x, 4)
end

return DebugOverlaySummary
