local DebugOverlaySummary = {}

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

DebugOverlaySummary.draw = function(self, engine)
	local lines = {}
	local layer_count = 0
	for _ in pairs(engine.layers) do
		layer_count += 1
	end
	local fps = engine.clock and engine.clock.delta and engine.clock.delta > 0 and flr(1 / engine.clock.delta) or 0
	local draw_entry = get_scope_entry(engine.profiler, "engine.draw")
	local update_entry = get_scope_entry(engine.profiler, "engine.update")
	local hz_text = ""
	if update_entry and update_entry.hz and update_entry.hz > 0 then
		hz_text ..= " upd:" .. flr(update_entry.hz + 0.5)
	end
	if draw_entry and draw_entry.hz and draw_entry.hz > 0 then
		hz_text ..= " draw:" .. flr(draw_entry.hz + 0.5)
	end
	self:appendLine(lines, "fps:" .. fps .. hz_text .. " layers:" .. layer_count)
	if engine.master_camera then
		self:appendLine(
			lines,
			"camera(" .. flr(engine.master_camera.pos.x) .. "," .. flr(engine.master_camera.pos.y) .. ")"
		)
	end

	for _, layer in pairs(engine.layers) do
		if layer.running and self.drawLayerGuides then
			self:drawLayerGuides(layer)
		end
		local layer_lines = self:collectLayerLines(layer)
		for _, line in pairs(layer_lines) do
			add(lines, line)
		end
	end

	self:drawLines(lines, 1, 1, 8)

	local gfx = get_overlay_gfx(engine)
	if gfx and engine.profiler then
		local perf_y = #lines * 6 + 4
		gfx:drawPerfPanel(engine.profiler, 1, perf_y, 238)
	end
end

return DebugOverlaySummary
