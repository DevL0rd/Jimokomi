local Class = include("src/Engine/Core/Class.lua")

local function round_ms(value)
	return flr(value * 1000 + 0.5) / 1000
end

local function round_value(value)
	return flr(value * 100 + 0.5) / 100
end

local function starts_with(value, prefix)
	return sub(value, 1, #prefix) == prefix
end

local function sort_strings(values)
	for i = 1, #values - 1 do
		local best_index = i
		for j = i + 1, #values do
			if tostring(values[j]) < tostring(values[best_index]) then
				best_index = j
			end
		end
		if best_index ~= i then
			values[i], values[best_index] = values[best_index], values[i]
		end
	end
end

local function ensure_scope(self, scope_name)
	local scope = self.scopes[scope_name]
	if not scope then
		scope = {
			total_ms = 0,
			max_ms = 0,
			total_cpu = 0,
			max_cpu = 0,
			calls = 0,
			counter = 0,
			observed_total = 0,
			observed_count = 0,
			observed_max = 0,
		}
		self.scopes[scope_name] = scope
	end
	return scope
end

local function matches_prefixes(name, prefixes)
	for i = 1, #prefixes do
		if starts_with(name, prefixes[i]) then
			return true
		end
	end
	return false
end

local function get_entry_by_name(entries, name)
	for i = 1, #(entries or {}) do
		local entry = entries[i]
		if entry.name == name then
			return entry
		end
	end
	return nil
end

local function get_entry_total_cpu(entries, name)
	local entry = get_entry_by_name(entries, name)
	return entry and round_value(entry.total_cpu or 0) or 0
end

local function get_scope_counter(entries, name)
	local entry = get_entry_by_name(entries, name)
	return entry and (entry.counter or 0) or 0
end

local function append_text_file(path, text, max_chars)
	local existing = fetch(path)
	if type(existing) ~= "string" then
		existing = ""
	end
	local next_text = existing
	if next_text ~= "" then
		next_text ..= "\n"
	end
	next_text ..= text
	if max_chars and #next_text > max_chars then
		next_text = sub(next_text, max(1, #next_text - max_chars + 1))
	end
	store(path, next_text)
end

local function get_runtime_cache_mode_label(runtime_stats)
	local mode = runtime_stats and runtime_stats.render_cache_mode or nil
	if mode == 1 then
		return "auto"
	end
	if mode == 2 then
		return "off"
	end
	if mode == 3 or mode == 4 or mode == 5 then
		return "on"
	end
	if mode == "auto" or mode == "on" or mode == "off" then
		return mode
	end
	return ((runtime_stats and runtime_stats.render_cache_enabled or 0) > 0) and "on" or "off"
end

local Profiler = Class:new({
	_type = "Profiler",
	report_interval_ms = 5000,
	log_path = "/appdata/jimokomi_perf_latest.txt",
	history_log_path = "/appdata/jimokomi_perf_history.txt",
	summary_log_path = "/appdata/jimokomi_perf_windows.txt",
	enabled = false,
	timing_enabled = false,
	keep_history = false,
	max_report_entries = 40,
	max_history_chars = 200000,
	max_summary_chars = 60000,
	scopes = nil,
	last_report_lines = nil,
	last_report_entries = nil,
	last_window_elapsed_ms = 0,
	report_serial = 0,
	history_by_scope = nil,
	history_scope_order = nil,
	history_limit = 24,

	init = function(self)
		self.scopes = {}
		self.last_report_lines = {}
		self.last_report_entries = {}
		self.report_serial = 0
		self.history_by_scope = {}
		self.history_scope_order = {}
		self.window_started_at = time()
		self.runtime_stats = {
			fps = 0,
			cpu = 0,
			draw_fps = 0,
			render_cache_enabled = 0,
			render_cache_mode = 2,
			auto_cache_known = 0,
			auto_cache_cached = 0,
			auto_cache_live = 0,
			auto_cache_total = 0,
			auto_cache_completed = 0,
			auto_cache_pending = 0,
			auto_cache_waiting = 0,
			auto_cache_active = 0,
			auto_cache_pending_samples = 0,
			auto_cache_active_samples = 0,
			auto_cache_active_key = "",
			auto_cache_active_bucket = "",
			auto_cache_queue_eta_s = 0,
			render_cache_entries = 0,
			render_cache_command_entries = 0,
			render_cache_surface_entries = 0,
			render_cache_total_commands = 0,
			render_cache_surface_area_total = 0,
			auto_cache_recent_lines = {},
			debug_text_cache_enabled = 0,
			debug_shape_cache_enabled = 0,
			debug_text_cache_toggle_serial = 0,
			debug_shape_cache_toggle_serial = 0,
			draw_calls = 0,
			last_draw_ms = 0,
			last_update_ms = 0,
			cpu_max = 0,
			cpu_spikes_over_08 = 0,
			cpu_spikes_over_10 = 0,
			frame_dt_ms_max = 0,
			frame_dt_ms_avg = 0,
			frame_count = 0,
		}
	end,

	setEnabled = function(self, enabled)
		local next_enabled = enabled == true
		if self.enabled == next_enabled then
			return
		end
		self.enabled = next_enabled
		self.timing_enabled = next_enabled
		self.keep_history = next_enabled
		self:resetWindow()
		if not next_enabled then
			self.last_report_lines = {}
			self.last_report_entries = {}
			self.last_window_elapsed_ms = 0
		end
	end,

	setRuntimeStat = function(self, name, value)
		if not self.enabled then
			return
		end
		self.runtime_stats = self.runtime_stats or {}
		self.runtime_stats[name] = value
	end,

	observeFrame = function(self, dt, cpu)
		if not self.enabled then
			return
		end
		self.runtime_stats = self.runtime_stats or {}
		local stats = self.runtime_stats
		local dt_ms = max(0, (dt or 0) * 1000)
		stats.frame_count = (stats.frame_count or 0) + 1
		stats.frame_dt_ms_avg = ((stats.frame_dt_ms_avg or 0) * (stats.frame_count - 1) + dt_ms) / stats.frame_count
		if dt_ms > (stats.frame_dt_ms_max or 0) then
			stats.frame_dt_ms_max = dt_ms
		end
		local safe_cpu = cpu or 0
		if safe_cpu > (stats.cpu_max or 0) then
			stats.cpu_max = safe_cpu
		end
		if safe_cpu >= 0.8 then
			stats.cpu_spikes_over_08 = (stats.cpu_spikes_over_08 or 0) + 1
		end
		if safe_cpu >= 1.0 then
			stats.cpu_spikes_over_10 = (stats.cpu_spikes_over_10 or 0) + 1
		end
	end,

	start = function(self, scope_name)
		if not self.enabled or not self.timing_enabled then
			return scope_name
		end
		return {
			name = scope_name,
			started_at = time(),
			started_cpu = stat and stat(1) or nil,
		}
	end,

	stop = function(self, token)
		if not self.enabled then
			return 0
		end
		if not token then
			return 0
		end
		if type(token) == "string" then
			local scope = ensure_scope(self, token)
			scope.calls += 1
			return 0
		end
		if not token.name or not token.started_at then
			return 0
		end
		local elapsed_ms = (time() - token.started_at) * 1000
		local scope = ensure_scope(self, token.name)
		local current_cpu = stat and stat(1) or nil
		local elapsed_cpu = 0
		if current_cpu ~= nil and token.started_cpu ~= nil then
			elapsed_cpu = max(0, current_cpu - token.started_cpu)
		end
		scope.total_ms += elapsed_ms
		scope.total_cpu += elapsed_cpu
		scope.calls += 1
		if elapsed_ms > scope.max_ms then
			scope.max_ms = elapsed_ms
		end
		if elapsed_cpu > scope.max_cpu then
			scope.max_cpu = elapsed_cpu
		end
		return elapsed_ms
	end,

	addCounter = function(self, scope_name, amount)
		if not self.enabled then
			return
		end
		local scope = ensure_scope(self, scope_name)
		scope.counter += amount or 1
	end,

	observe = function(self, scope_name, value)
		if not self.enabled then
			return
		end
		local scope = ensure_scope(self, scope_name)
		scope.observed_total += value
		scope.observed_count += 1
		if value > scope.observed_max then
			scope.observed_max = value
		end
	end,

	resetWindow = function(self)
		self.scopes = {}
		self.window_started_at = time()
		local fps = self.runtime_stats and self.runtime_stats.fps or 0
		local cpu = self.runtime_stats and self.runtime_stats.cpu or 0
		local draw_fps = self.runtime_stats and self.runtime_stats.draw_fps or 0
		local render_cache_enabled = self.runtime_stats and self.runtime_stats.render_cache_enabled or 0
		local render_cache_mode = self.runtime_stats and self.runtime_stats.render_cache_mode or 2
		local auto_cache_known = self.runtime_stats and self.runtime_stats.auto_cache_known or 0
		local auto_cache_cached = self.runtime_stats and self.runtime_stats.auto_cache_cached or 0
		local auto_cache_live = self.runtime_stats and self.runtime_stats.auto_cache_live or 0
		local auto_cache_total = self.runtime_stats and self.runtime_stats.auto_cache_total or 0
		local auto_cache_completed = self.runtime_stats and self.runtime_stats.auto_cache_completed or 0
		local auto_cache_pending = self.runtime_stats and self.runtime_stats.auto_cache_pending or 0
		local auto_cache_waiting = self.runtime_stats and self.runtime_stats.auto_cache_waiting or 0
		local auto_cache_active = self.runtime_stats and self.runtime_stats.auto_cache_active or 0
		local auto_cache_pending_samples = self.runtime_stats and self.runtime_stats.auto_cache_pending_samples or 0
		local auto_cache_active_samples = self.runtime_stats and self.runtime_stats.auto_cache_active_samples or 0
		local auto_cache_active_key = self.runtime_stats and self.runtime_stats.auto_cache_active_key or ""
		local auto_cache_active_bucket = self.runtime_stats and self.runtime_stats.auto_cache_active_bucket or ""
		local auto_cache_queue_eta_s = self.runtime_stats and self.runtime_stats.auto_cache_queue_eta_s or 0
		local render_cache_entries = self.runtime_stats and self.runtime_stats.render_cache_entries or 0
		local render_cache_command_entries = self.runtime_stats and self.runtime_stats.render_cache_command_entries or 0
		local render_cache_surface_entries = self.runtime_stats and self.runtime_stats.render_cache_surface_entries or 0
		local render_cache_total_commands = self.runtime_stats and self.runtime_stats.render_cache_total_commands or 0
		local render_cache_surface_area_total = self.runtime_stats and self.runtime_stats.render_cache_surface_area_total or 0
		local auto_cache_recent_lines = self.runtime_stats and self.runtime_stats.auto_cache_recent_lines or {}
		local debug_text_cache_enabled = self.runtime_stats and self.runtime_stats.debug_text_cache_enabled or 0
		local debug_shape_cache_enabled = self.runtime_stats and self.runtime_stats.debug_shape_cache_enabled or 0
		local debug_text_cache_toggle_serial = self.runtime_stats and self.runtime_stats.debug_text_cache_toggle_serial or 0
		local debug_shape_cache_toggle_serial = self.runtime_stats and self.runtime_stats.debug_shape_cache_toggle_serial or 0
		self.runtime_stats = {
			fps = fps,
			cpu = cpu,
			draw_fps = draw_fps,
			render_cache_enabled = render_cache_enabled,
			render_cache_mode = render_cache_mode,
			auto_cache_known = auto_cache_known,
			auto_cache_cached = auto_cache_cached,
			auto_cache_live = auto_cache_live,
			auto_cache_total = auto_cache_total,
			auto_cache_completed = auto_cache_completed,
			auto_cache_pending = auto_cache_pending,
			auto_cache_waiting = auto_cache_waiting,
			auto_cache_active = auto_cache_active,
			auto_cache_pending_samples = auto_cache_pending_samples,
			auto_cache_active_samples = auto_cache_active_samples,
			auto_cache_active_key = auto_cache_active_key,
			auto_cache_active_bucket = auto_cache_active_bucket,
			auto_cache_queue_eta_s = auto_cache_queue_eta_s,
			render_cache_entries = render_cache_entries,
			render_cache_command_entries = render_cache_command_entries,
			render_cache_surface_entries = render_cache_surface_entries,
			render_cache_total_commands = render_cache_total_commands,
			render_cache_surface_area_total = render_cache_surface_area_total,
			auto_cache_recent_lines = auto_cache_recent_lines,
			debug_text_cache_enabled = debug_text_cache_enabled,
			debug_shape_cache_enabled = debug_shape_cache_enabled,
			debug_text_cache_toggle_serial = debug_text_cache_toggle_serial,
			debug_shape_cache_toggle_serial = debug_shape_cache_toggle_serial,
			draw_calls = 0,
			last_draw_ms = self.runtime_stats and self.runtime_stats.last_draw_ms or 0,
			last_update_ms = self.runtime_stats and self.runtime_stats.last_update_ms or 0,
			cpu_max = 0,
			cpu_spikes_over_08 = 0,
			cpu_spikes_over_10 = 0,
			frame_dt_ms_max = 0,
			frame_dt_ms_avg = 0,
			frame_count = 0,
		}
	end,

	buildReportEntries = function(self)
		local names = {}
		for name in pairs(self.scopes) do
			add(names, name)
		end
		sort_strings(names)

		local entries = {}
		for i = 1, #names do
			local name = names[i]
			local scope = self.scopes[name]
			local avg_ms = scope.calls > 0 and scope.total_ms / scope.calls or 0
			local avg_cpu = scope.calls > 0 and scope.total_cpu / scope.calls or 0
			add(entries, {
				name = name,
				total_ms = scope.total_ms,
				avg_ms = avg_ms,
				max_ms = scope.max_ms,
				total_cpu = scope.total_cpu,
				avg_cpu = avg_cpu,
				max_cpu = scope.max_cpu,
				calls = scope.calls,
				counter = scope.counter,
				activity_score = scope.total_cpu * 1000 + scope.total_ms + scope.calls * 0.01 + scope.counter * 0.01 + scope.observed_total * 0.01,
				observed_total = scope.observed_total,
				observed_avg = scope.observed_count > 0 and scope.observed_total / scope.observed_count or 0,
				observed_max = scope.observed_max,
				observed_count = scope.observed_count,
				hz = 0,
			})
		end

		for i = 1, #entries - 1 do
			local best_index = i
			for j = i + 1, #entries do
				if entries[j].activity_score > entries[best_index].activity_score then
					best_index = j
				end
			end
			if best_index ~= i then
				entries[i], entries[best_index] = entries[best_index], entries[i]
			end
		end

		return entries
	end,

	pushHistoryEntry = function(self, name, value)
		local history = self.history_by_scope[name]
		if not history then
			history = {}
			self.history_by_scope[name] = history
			add(self.history_scope_order, name)
		end
		add(history, value)
		while #history > self.history_limit do
			deli(history, 1)
		end
	end,

	buildReportLines = function(self, elapsed_ms, entries)
		local lines = {
			"perf window:" .. flr(elapsed_ms) .. "ms",
		}
		local runtime_stats = self.runtime_stats or {}
		if runtime_stats.fps or runtime_stats.cpu or runtime_stats.draw_fps then
			add(
				lines,
				"picotron fps:" .. round_value(runtime_stats.fps or 0) ..
				" draw:" .. round_value(runtime_stats.draw_fps or 0) ..
				" cpu:" .. round_value(runtime_stats.cpu or 0)
			)
			add(
				lines,
				"cache mode:r:" .. get_runtime_cache_mode_label(runtime_stats) ..
				" t:" .. ((runtime_stats.debug_text_cache_enabled or 0) > 0 and "on" or "off") ..
				" s:" .. ((runtime_stats.debug_shape_cache_enabled or 0) > 0 and "on" or "off") ..
				" tog:" .. tostr(runtime_stats.debug_text_cache_toggle_serial or 0) ..
				"/" .. tostr(runtime_stats.debug_shape_cache_toggle_serial or 0)
			)
			if (runtime_stats.auto_cache_total or 0) > 0 or (runtime_stats.auto_cache_known or 0) > 0 then
				add(
					lines,
					"auto cache known:" .. tostr(runtime_stats.auto_cache_known or 0) ..
					" cached:" .. tostr(runtime_stats.auto_cache_cached or 0) ..
					" live:" .. tostr(runtime_stats.auto_cache_live or 0)
				)
				add(
					lines,
					"auto queue done:" .. tostr(runtime_stats.auto_cache_completed or 0) ..
					"/" .. tostr(runtime_stats.auto_cache_total or 0) ..
					" pending:" .. tostr(runtime_stats.auto_cache_pending or 0) ..
					" waiting:" .. tostr(runtime_stats.auto_cache_waiting or 0) ..
					" active:" .. tostr(runtime_stats.auto_cache_active or 0) ..
					" samples:" .. tostr(runtime_stats.auto_cache_pending_samples or 0) ..
					" eta_s:" .. round_value(runtime_stats.auto_cache_queue_eta_s or 0)
				)
				if runtime_stats.auto_cache_active_bucket and runtime_stats.auto_cache_active_bucket ~= "" then
					add(
						lines,
						"auto active:" .. tostr(runtime_stats.auto_cache_active_bucket or "?") ..
						" left:" .. tostr(runtime_stats.auto_cache_active_samples or 0)
					)
				end
				add(
					lines,
					"render cache entries:" .. tostr(runtime_stats.render_cache_entries or 0) ..
					" command:" .. tostr(runtime_stats.render_cache_command_entries or 0) ..
					" surface:" .. tostr(runtime_stats.render_cache_surface_entries or 0) ..
					" cmd_total:" .. tostr(runtime_stats.render_cache_total_commands or 0) ..
					" surface_area:" .. tostr(runtime_stats.render_cache_surface_area_total or 0)
				)
				for i = 1, #(runtime_stats.auto_cache_recent_lines or {}) do
					add(lines, "auto " .. i .. ": " .. tostr(runtime_stats.auto_cache_recent_lines[i]))
				end
			end
			add(
				lines,
				"spikes cpu_max:" .. round_value(runtime_stats.cpu_max or 0) ..
				" dt_max:" .. round_value(runtime_stats.frame_dt_ms_max or 0) ..
				" dt_avg:" .. round_value(runtime_stats.frame_dt_ms_avg or 0) ..
				" >0.8:" .. tostr(runtime_stats.cpu_spikes_over_08 or 0) ..
				" >1.0:" .. tostr(runtime_stats.cpu_spikes_over_10 or 0)
			)
		end
		local elapsed_s = max(0.001, elapsed_ms / 1000)

		local function append_entry(entry)
			local line = entry.name ..
				" t:" .. round_ms(entry.total_ms) ..
				" avg:" .. round_ms(entry.avg_ms) ..
				" max:" .. round_ms(entry.max_ms) ..
				" cpu:" .. round_value(entry.total_cpu) ..
				"/" .. round_value(entry.avg_cpu) ..
				"/" .. round_value(entry.max_cpu) ..
				" c:" .. entry.calls ..
				" hz:" .. round_value(entry.calls / elapsed_s)
			if entry.counter > 0 then
				line ..= " n:" .. entry.counter
			end
			if entry.observed_count > 0 then
				line ..= " obs:" .. round_ms(entry.observed_avg) ..
					"/" .. round_ms(entry.observed_max)
			end
			add(lines, line)
		end

		local remaining = {}
		local used_names = {}

		local function mark_used(entry)
			used_names[entry.name] = true
		end

		local function append_full_section(title, prefixes)
			local section_entries = {}
			for i = 1, #entries do
				local entry = entries[i]
				if not used_names[entry.name] and matches_prefixes(entry.name, prefixes) then
					add(section_entries, entry)
				end
			end
			if #section_entries == 0 then
				return
			end
			add(lines, "[" .. title .. "]")
			for i = 1, #section_entries do
				append_entry(section_entries[i])
				mark_used(section_entries[i])
			end
		end

		append_full_section("entity_update_types", { "layer.sim.update_entities.type." })
		append_full_section("entity_update_objects", { "layer.sim.update_entities.object." })
		append_full_section("entity_draw_types", { "layer.render.entities.type.", "layer.render.entities_behind_map.type." })
		append_full_section("entity_draw_objects", { "layer.render.entities.object.", "layer.render.entities_behind_map.object." })

		for i = 1, #entries do
			local entry = entries[i]
			if not used_names[entry.name] then
				add(remaining, entry)
				if #remaining >= (self.max_report_entries or 40) then
					break
				end
			end
		end

		local function append_section(title, prefixes)
			local wrote_header = false
			for i = #remaining, 1, -1 do
				local entry = remaining[i]
				if matches_prefixes(entry.name, prefixes) then
					if not wrote_header then
						add(lines, "[" .. title .. "]")
						wrote_header = true
					end
					append_entry(entry)
					deli(remaining, i)
				end
			end
		end

		append_section("update", {
			"engine.update",
			"layer.sim.",
			"game.agent.",
			"game.pathing.",
			"world.senses.",
			"world.raycast.",
			"world.pathfinder.",
			"collision.",
		})
		append_section("draw", {
			"engine.draw",
			"engine.debug_overlay.draw",
			"layer.render.",
			"layer.",
			"picotron.",
		})
		append_section("cache", {
			"render.cache.",
			"render.panel_cache.",
			"render.retained.",
		})
		if #remaining > 0 then
			add(lines, "[other]")
			for i = 1, #remaining do
				append_entry(remaining[i])
			end
		end

		return lines
	end,

	buildCompactSummaryLine = function(self, elapsed_ms, entries)
		local runtime_stats = self.runtime_stats or {}
		local window_s = round_value(max(0, elapsed_ms / 1000))
		local mode_line =
			"r:" .. get_runtime_cache_mode_label(runtime_stats) ..
			" t:" .. ((runtime_stats.debug_text_cache_enabled or 0) > 0 and "on" or "off") ..
			" s:" .. ((runtime_stats.debug_shape_cache_enabled or 0) > 0 and "on" or "off")
		return
			"report:" .. tostr(self.report_serial or 0) ..
			" time:" .. tostr(time()) ..
			" window_s:" .. tostr(window_s) ..
			" mode:" .. mode_line ..
			" toggles:" .. tostr(runtime_stats.debug_text_cache_toggle_serial or 0) ..
			"/" .. tostr(runtime_stats.debug_shape_cache_toggle_serial or 0) ..
			" fps:" .. tostr(round_value(runtime_stats.fps or 0)) ..
			" cpu:" .. tostr(round_value(runtime_stats.cpu or 0)) ..
			" draw_cpu:" .. tostr(get_entry_total_cpu(entries, "engine.draw")) ..
			" map_cpu:" .. tostr(get_entry_total_cpu(entries, "layer.render.map")) ..
			" entities_cpu:" .. tostr(get_entry_total_cpu(entries, "layer.render.entities")) ..
			" overlay_cpu:" .. tostr(get_entry_total_cpu(entries, "engine.debug_overlay.draw")) ..
			" guides_cpu:" .. tostr(get_entry_total_cpu(entries, "engine.debug_overlay.guides")) ..
			" grid_cpu:" .. tostr(get_entry_total_cpu(entries, "layer.render.debug_grid")) ..
			" auto_known:" .. tostr(runtime_stats.auto_cache_known or 0) ..
			" auto_done:" .. tostr(runtime_stats.auto_cache_completed or 0) ..
			"/" .. tostr(runtime_stats.auto_cache_total or 0) ..
			" auto_cached:" .. tostr(runtime_stats.auto_cache_cached or 0) ..
			" auto_live:" .. tostr(runtime_stats.auto_cache_live or 0) ..
			" auto_pending:" .. tostr(runtime_stats.auto_cache_pending or 0) ..
			" auto_active:" .. tostr(runtime_stats.auto_cache_active or 0) ..
			" auto_eta_s:" .. tostr(round_value(runtime_stats.auto_cache_queue_eta_s or 0)) ..
			" cache_entries:" .. tostr(runtime_stats.render_cache_entries or 0) ..
			" cache_cmd_entries:" .. tostr(runtime_stats.render_cache_command_entries or 0) ..
			" cache_surface_entries:" .. tostr(runtime_stats.render_cache_surface_entries or 0) ..
			" cache_hits:" .. tostr(get_scope_counter(entries, "render.cache.hits")) ..
			" cache_replays:" .. tostr(get_scope_counter(entries, "render.cache.replays")) ..
			" cache_bypass:" .. tostr(get_scope_counter(entries, "render.cache.bypass")) ..
			" panel_hits:" .. tostr(get_scope_counter(entries, "render.panel_cache.hits")) ..
			" retained_hits:" .. tostr(get_scope_counter(entries, "render.retained.hits"))
	end,

	reportIfDue = function(self)
		if not self.enabled then
			return false
		end
		local elapsed_ms = (time() - self.window_started_at) * 1000
		if elapsed_ms < self.report_interval_ms then
			return false
		end

		self.last_report_entries = self:buildReportEntries()
		self.last_window_elapsed_ms = elapsed_ms
		self.report_serial += 1
		local elapsed_s = max(0.001, elapsed_ms / 1000)
		for i = 1, #self.last_report_entries do
			local entry = self.last_report_entries[i]
			entry.hz = entry.calls / elapsed_s
			if self.keep_history then
				self:pushHistoryEntry(entry.name, entry.total_cpu > 0 and entry.total_cpu or entry.total_ms)
			end
		end
		self.last_report_lines = self:buildReportLines(elapsed_ms, self.last_report_entries)
		local report = table.concat(self.last_report_lines, "\n")

		pcall(function()
			store(self.log_path, report)
		end)
		pcall(function()
			append_text_file(
				self.history_log_path,
				"==== report:" .. tostr(self.report_serial) .. " time:" .. tostr(time()) .. " ====\n" .. report,
				self.max_history_chars
			)
		end)
		pcall(function()
			append_text_file(
				self.summary_log_path,
				self:buildCompactSummaryLine(elapsed_ms, self.last_report_entries),
				self.max_summary_chars
			)
		end)

		self:resetWindow()
		return true
	end,
})

return Profiler
