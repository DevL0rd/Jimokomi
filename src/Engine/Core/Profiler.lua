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

local Profiler = Class:new({
	_type = "Profiler",
	report_interval_ms = 5000,
	log_path = "/appdata/jimokomi_perf_latest.txt",
	enabled = false,
	timing_enabled = false,
	keep_history = false,
	max_report_entries = 40,
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
			draw_calls = 0,
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
		self.runtime_stats = {
			fps = fps,
			cpu = cpu,
			draw_fps = draw_fps,
			draw_calls = 0,
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
		for i = 1, #entries do
			if i > (self.max_report_entries or 40) then
				break
			end
			add(remaining, entries[i])
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

		self:resetWindow()
		return true
	end,
})

return Profiler
