local Class = include("src/Engine/Core/Class.lua")

local function round_ms(value)
	return flr(value * 1000 + 0.5) / 1000
end

local function round_value(value)
	return flr(value * 100 + 0.5) / 100
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

local Profiler = Class:new({
	_type = "Profiler",
	report_interval_ms = 5000,
	log_path = "/appdata/jimokomi_perf_latest.txt",
	timing_enabled = false,
	keep_history = false,
	max_report_entries = 40,
	scopes = nil,
	last_report_lines = nil,
	last_report_entries = nil,
	last_window_elapsed_ms = 0,
	history_by_scope = nil,
	history_scope_order = nil,
	history_limit = 24,

	init = function(self)
		self.scopes = {}
		self.last_report_lines = {}
		self.last_report_entries = {}
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

	setRuntimeStat = function(self, name, value)
		self.runtime_stats = self.runtime_stats or {}
		self.runtime_stats[name] = value
	end,

	observeFrame = function(self, dt, cpu)
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
		if not self.timing_enabled then
			return scope_name
		end
		return {
			name = scope_name,
			started_at = time(),
		}
	end,

	stop = function(self, token)
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
		scope.total_ms += elapsed_ms
		scope.calls += 1
		if elapsed_ms > scope.max_ms then
			scope.max_ms = elapsed_ms
		end
		return elapsed_ms
	end,

	addCounter = function(self, scope_name, amount)
		local scope = ensure_scope(self, scope_name)
		scope.counter += amount or 1
	end,

	observe = function(self, scope_name, value)
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
			add(entries, {
				name = name,
				total_ms = scope.total_ms,
				avg_ms = avg_ms,
				max_ms = scope.max_ms,
				calls = scope.calls,
				counter = scope.counter,
				activity_score = scope.total_ms + scope.calls * 0.01 + scope.counter * 0.01 + scope.observed_total * 0.01,
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

		for i = 1, #entries do
			if i > (self.max_report_entries or 40) then
				break
			end
			local entry = entries[i]
			local line = entry.name ..
				" t:" .. round_ms(entry.total_ms) ..
				" avg:" .. round_ms(entry.avg_ms) ..
				" max:" .. round_ms(entry.max_ms) ..
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

		return lines
	end,

	reportIfDue = function(self)
		local elapsed_ms = (time() - self.window_started_at) * 1000
		if elapsed_ms < self.report_interval_ms then
			return false
		end

		self.last_report_entries = self:buildReportEntries()
		self.last_window_elapsed_ms = elapsed_ms
		local elapsed_s = max(0.001, elapsed_ms / 1000)
		for i = 1, #self.last_report_entries do
			local entry = self.last_report_entries[i]
			entry.hz = entry.calls / elapsed_s
			if self.keep_history then
				self:pushHistoryEntry(entry.name, entry.total_ms)
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
