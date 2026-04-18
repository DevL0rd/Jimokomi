local DebugOverlay = include("src/Engine/Core/DebugOverlay.lua")

local EngineLoop = {}

_dt = 0

local function update_timed_toggle(self, now_ms, auto_flag_name, enabled_flag_name, interval_name, next_name, serial_name, setter_name)
	if self[auto_flag_name] ~= true then
		self[next_name] = nil
		return false
	end

	local interval_ms = max(1, flr(self[interval_name] or 10000))
	local next_toggle_ms = self[next_name]
	if next_toggle_ms == nil then
		self[next_name] = now_ms + interval_ms
		return false
	end
	if now_ms < next_toggle_ms then
		return false
	end

	local toggles_due = 1 + flr((now_ms - next_toggle_ms) / interval_ms)
	for _ = 1, toggles_due do
		local next_enabled = not (self[enabled_flag_name] == true)
		self[setter_name](self, next_enabled)
		self[serial_name] = (self[serial_name] or 0) + 1
	end
	self[next_name] = next_toggle_ms + toggles_due * interval_ms
	return toggles_due > 0
end

local function round_signed_thousandths(value)
	return flr(value * 1000 + (value >= 0 and 0.5 or -0.5)) / 1000
end

local function format_auto_cache_recent_line(entry)
	local cpu_delta = round_signed_thousandths(entry and entry.cpu_delta or 0)
	local ms_delta = round_signed_thousandths(entry and entry.ms_delta or 0)
	local cpu_prefix = cpu_delta >= 0 and "+" or ""
	local ms_prefix = ms_delta >= 0 and "+" or ""
	return
		tostr(entry.choice or "?") ..
		"(" .. tostr(entry.mode or "?") .. ") " ..
		tostr(entry.tag or "-") ..
		" key:" .. tostr(entry.key or "anonymous") ..
		" s:" .. tostr(entry.immediate_samples or 0) .. "/" .. tostr(entry.cached_samples or 0) ..
		" d_cpu:" .. cpu_prefix .. tostr(cpu_delta) ..
		" d_ms:" .. ms_prefix .. tostr(ms_delta)
end

local function merge_auto_cache_snapshot(into, snapshot, limit)
	if not snapshot then
		return
	end
	into.known = (into.known or 0) + (snapshot.known or 0)
	into.cached = (into.cached or 0) + (snapshot.cached or 0)
	into.live = (into.live or 0) + (snapshot.live or 0)
	into.total = (into.total or 0) + (snapshot.total or 0)
	into.completed = (into.completed or 0) + (snapshot.completed or 0)
	into.pending = (into.pending or 0) + (snapshot.pending or 0)
	into.waiting = (into.waiting or 0) + (snapshot.waiting or 0)
	into.active = (into.active or 0) + (snapshot.active or 0)
	into.pending_samples = (into.pending_samples or 0) + (snapshot.pending_samples or 0)
	into.active_samples = (into.active_samples or 0) + (snapshot.active_samples or 0)
	if into.active_key == nil and snapshot.active_key ~= nil then
		into.active_key = snapshot.active_key
	end
	if into.active_bucket == nil and snapshot.active_bucket ~= nil then
		into.active_bucket = snapshot.active_bucket
	end
	for i = 1, #(snapshot.recent or {}) do
		local candidate = snapshot.recent[i]
		local insert_at = #(into.recent or {}) + 1
		for j = 1, #(into.recent or {}) do
			if (candidate.serial or 0) > (into.recent[j].serial or 0) then
				insert_at = j
				break
			end
		end
		add(into.recent, candidate, insert_at)
		while #into.recent > (limit or 5) do
			deli(into.recent, #into.recent)
		end
	end
end

local function merge_render_cache_snapshot(into, snapshot)
	if not snapshot then
		return
	end
	into.entries = (into.entries or 0) + (snapshot.entries or 0)
	into.command_entries = (into.command_entries or 0) + (snapshot.command_entries or 0)
	into.surface_entries = (into.surface_entries or 0) + (snapshot.surface_entries or 0)
	into.total_commands = (into.total_commands or 0) + (snapshot.total_commands or 0)
	into.surface_area = (into.surface_area or 0) + (snapshot.surface_area or 0)
end

local function collect_auto_cache_runtime_stats(self, limit)
	local snapshot = {
		known = 0,
		cached = 0,
		live = 0,
		total = 0,
		completed = 0,
		pending = 0,
		waiting = 0,
		active = 0,
		pending_samples = 0,
		active_samples = 0,
		active_key = nil,
		active_bucket = nil,
		recent = {},
	}
	for i = 1, #self.sorted_layer_ids do
		local layer = self.layers[self.sorted_layer_ids[i]]
		local gfx = layer and layer.gfx or nil
		if gfx and gfx.getAutoCacheDecisionSnapshot then
			merge_auto_cache_snapshot(snapshot, gfx:getAutoCacheDecisionSnapshot(limit or 5), limit or 5)
		end
	end
	local recent_lines = {}
	for i = 1, #snapshot.recent do
		add(recent_lines, format_auto_cache_recent_line(snapshot.recent[i]))
	end
	snapshot.recent_lines = recent_lines
	return snapshot
end

local function collect_render_cache_runtime_stats(self)
	local snapshot = {
		entries = 0,
		command_entries = 0,
		surface_entries = 0,
		total_commands = 0,
		surface_area = 0,
	}
	for i = 1, #self.sorted_layer_ids do
		local layer = self.layers[self.sorted_layer_ids[i]]
		local gfx = layer and layer.gfx or nil
		if gfx and gfx.getRenderCacheSnapshot then
			merge_render_cache_snapshot(snapshot, gfx:getRenderCacheSnapshot())
		end
	end
	return snapshot
end

EngineLoop.update = function(self)
	self:ensureServices()
	if self.clock.last_time == 0 then
		self.clock:start()
	end

	local measured_dt = self.clock:tick()
	local fixed_update_hz = max(1, self.fixed_update_hz or 30)
	local fixed_dt = 1 / fixed_update_hz
	local max_update_steps = max(1, self.max_update_steps_per_frame or 4)
	local max_accumulator = fixed_dt * max_update_steps
	self.update_accumulator = min((self.update_accumulator or 0) + measured_dt, max_accumulator)
	if self.update_accumulator < fixed_dt then
		return
	end

	local now_ms = flr((time() or 0) * 1000 + 0.5)
	local steps_run = 0
	local update_cpu_accum = 0

	while self.update_accumulator >= fixed_dt and steps_run < max_update_steps do
		_dt = fixed_dt
		local update_cpu_start = stat and stat(1) or 0
		local update_scope = self.profiler and self.profiler:start("engine.update") or nil
		local toggled_debug_text_cache = update_timed_toggle(
			self,
			now_ms,
			"auto_toggle_debug_text_cache",
			"debug_text_cache_enabled",
			"debug_text_cache_toggle_interval_ms",
			"next_debug_text_cache_toggle_ms",
			"debug_text_cache_toggle_serial",
			"setDebugTextCacheEnabled"
		)
		local toggled_debug_shape_cache = update_timed_toggle(
			self,
			now_ms,
			"auto_toggle_debug_shape_cache",
			"debug_shape_cache_enabled",
			"debug_shape_cache_toggle_interval_ms",
			"next_debug_shape_cache_toggle_ms",
			"debug_shape_cache_toggle_serial",
			"setDebugShapeCacheEnabled"
		)
		if self.profiler and (toggled_debug_text_cache or toggled_debug_shape_cache) then
			self.profiler:resetWindow()
		end
		self:clearDebug()

		for i = 1, #self.sorted_layer_ids do
			local layer_id = self.sorted_layer_ids[i]
			local layer = self.layers[layer_id]
			if layer.camera ~= self.master_camera and layer.camera.parallax_factor then
				layer.camera:linkToCamera(
					self.master_camera,
					layer.camera.parallax_factor.x,
					layer.camera.parallax_factor.y
				)
			end
			local layer_scope = self.profiler and self.profiler:start("layer." .. layer_id .. ".update") or nil
			layer:update()
			if self.profiler then
				self.profiler:stop(layer_scope)
			end
		end

		self.update_accumulator -= fixed_dt
		steps_run += 1
		local update_cpu_end = stat and stat(1) or 0
		update_cpu_accum += max(0, (update_cpu_end or 0) - (update_cpu_start or 0))
		if self.profiler then
			local update_ms = self.profiler:stop(update_scope)
			self.profiler:setRuntimeStat("last_update_ms", update_ms or 0)
			self.profiler:reportIfDue()
		end
	end

	if steps_run > 0 then
		self.last_update_cpu_frame = update_cpu_accum
	end
end

EngineLoop.draw = function(self)
	self:ensureServices()
	local draw_cpu_start = stat and stat(1) or 0
	local draw_scope = self.profiler and self.profiler:start("engine.draw") or nil
	local runtime_fps = stat and stat(7) or 0
	cls()

	if self.master_camera then
		local cam_x, cam_y = self.master_camera:layerToScreenXY(0, 0)

		if self.fill_color > -1 then
			rectfill(
				cam_x,
				cam_y,
				cam_x + self.w - 1,
				cam_y + self.h - 1,
				self.fill_color
			)
		end

		if self.stroke_color > -1 then
			rect(
				cam_x,
				cam_y,
				cam_x + self.w - 1,
				cam_y + self.h - 1,
				self.stroke_color
			)
		end
	end

	for i = 1, #self.sorted_layer_ids do
		local layer_id = self.sorted_layer_ids[i]
		local layer_scope = self.profiler and self.profiler:start("layer." .. layer_id .. ".draw") or nil
		self.layers[layer_id]:draw()
		if self.profiler then
			self.profiler:stop(layer_scope)
		end
	end

	if self.debug_overlay_enabled or self.performance_panel_enabled then
		if not self.debug_overlay then
			self.debug_overlay = DebugOverlay:new()
		end
		local overlay_scope = self.profiler and self.profiler:start("engine.debug_overlay.draw") or nil
		self.debug_overlay:draw(self)
		if self.profiler then
			self.profiler:stop(overlay_scope)
		end
	end

	camera()
	if self.profiler then
		local draw_ms = self.profiler:stop(draw_scope)
		local draw_cpu_end = stat and stat(1) or 0
		self.last_draw_cpu_frame = max(0, (draw_cpu_end or 0) - (draw_cpu_start or 0))
		local runtime_cpu = stat and stat(1) or 0
		local measured_dt = self.clock and self.clock.measured_delta or _dt or 0
		local auto_cache_stats = collect_auto_cache_runtime_stats(self, 5)
		local render_cache_stats = collect_render_cache_runtime_stats(self)
		local active_slots = max(1, auto_cache_stats.active or 0)
		local draw_rate = max(1, runtime_fps or 0)
		local auto_cache_queue_eta_s = (auto_cache_stats.pending_samples or 0) / (draw_rate * active_slots)
		self.profiler:setRuntimeStat("fps", runtime_fps or 0)
		self.profiler:setRuntimeStat("cpu", runtime_cpu or 0)
		self.profiler:setRuntimeStat("draw_fps", runtime_fps or 0)
		self.profiler:setRuntimeStat("last_draw_ms", draw_ms or 0)
		self.profiler:setRuntimeStat("render_cache_enabled", self.render_cache_enabled == true and 1 or 0)
		self.profiler:setRuntimeStat("render_cache_mode", self.render_cache_mode or (self.render_cache_enabled == true and "on" or "off"))
		self.profiler:setRuntimeStat("auto_cache_known", auto_cache_stats.known or 0)
		self.profiler:setRuntimeStat("auto_cache_cached", auto_cache_stats.cached or 0)
		self.profiler:setRuntimeStat("auto_cache_live", auto_cache_stats.live or 0)
		self.profiler:setRuntimeStat("auto_cache_total", auto_cache_stats.total or 0)
		self.profiler:setRuntimeStat("auto_cache_completed", auto_cache_stats.completed or 0)
		self.profiler:setRuntimeStat("auto_cache_pending", auto_cache_stats.pending or 0)
		self.profiler:setRuntimeStat("auto_cache_waiting", auto_cache_stats.waiting or 0)
		self.profiler:setRuntimeStat("auto_cache_active", auto_cache_stats.active or 0)
		self.profiler:setRuntimeStat("auto_cache_pending_samples", auto_cache_stats.pending_samples or 0)
		self.profiler:setRuntimeStat("auto_cache_active_samples", auto_cache_stats.active_samples or 0)
		self.profiler:setRuntimeStat("auto_cache_active_key", auto_cache_stats.active_key or "")
		self.profiler:setRuntimeStat("auto_cache_active_bucket", auto_cache_stats.active_bucket or "")
		self.profiler:setRuntimeStat("auto_cache_queue_eta_s", auto_cache_queue_eta_s or 0)
		self.profiler:setRuntimeStat("render_cache_entries", render_cache_stats.entries or 0)
		self.profiler:setRuntimeStat("render_cache_command_entries", render_cache_stats.command_entries or 0)
		self.profiler:setRuntimeStat("render_cache_surface_entries", render_cache_stats.surface_entries or 0)
		self.profiler:setRuntimeStat("render_cache_total_commands", render_cache_stats.total_commands or 0)
		self.profiler:setRuntimeStat("render_cache_surface_area_total", render_cache_stats.surface_area or 0)
		self.profiler:setRuntimeStat("auto_cache_recent_lines", auto_cache_stats.recent_lines or {})
		self.profiler:setRuntimeStat("debug_text_cache_enabled", self.debug_text_cache_enabled == true and 1 or 0)
		self.profiler:setRuntimeStat("debug_shape_cache_enabled", self.debug_shape_cache_enabled == true and 1 or 0)
		self.profiler:setRuntimeStat("debug_text_cache_toggle_serial", self.debug_text_cache_toggle_serial or 0)
		self.profiler:setRuntimeStat("debug_shape_cache_toggle_serial", self.debug_shape_cache_toggle_serial or 0)
		self.profiler:observeFrame(measured_dt, runtime_cpu or 0)
		self.profiler:addCounter("picotron.draw_calls", 1)
		self.profiler:observe("picotron.fps", runtime_fps or 0)
		self.profiler:observe("picotron.cpu", runtime_cpu or 0)
	end
end

EngineLoop.start = function(self)
	self:ensureServices()
	self.clock:start()
	for i = 1, #self.sorted_layer_ids do
		self.layers[self.sorted_layer_ids[i]]:start()
	end
end

EngineLoop.stop = function(self)
	for i = 1, #self.sorted_layer_ids do
		self.layers[self.sorted_layer_ids[i]]:stop()
	end
end

return EngineLoop
