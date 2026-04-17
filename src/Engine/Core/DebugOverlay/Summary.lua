local DebugOverlaySummary = {}

DebugOverlaySummary.draw = function(self, engine)
	local lines = {}
	local layer_count = 0
	local total_objects = 0
	local total_drawables = 0
	local total_collidables = 0
	local total_attached = 0
	for _ in pairs(engine.layers) do
		layer_count += 1
	end
	self:appendLine(lines, "engine layers:" .. layer_count)
	if engine.master_camera then
		self:appendLine(
			lines,
			"camera(" .. flr(engine.master_camera.pos.x) .. "," .. flr(engine.master_camera.pos.y) .. ")"
		)
	end

	for _, layer in pairs(engine.layers) do
		total_objects += #layer.entities
		total_drawables += #layer.drawable_entities
		total_collidables += #layer.collidable_entities
		total_attached += #layer.attached_entities
		local layer_lines = self:collectLayerLines(layer)
		for _, line in pairs(layer_lines) do
			add(lines, line)
		end
	end

	local fps = _dt and _dt > 0 and flr(1 / _dt) or 0
	self:appendLine(
		lines,
		"perf fps:" .. fps ..
		" obj:" .. total_objects ..
		" draw:" .. total_drawables ..
		" col:" .. total_collidables ..
		" att:" .. total_attached
	)

	if engine.debug_buffer and engine.debug_buffer.text and engine.debug_buffer.text ~= "" then
		for line in all(split(engine.debug_buffer.text, "\n", false)) do
			add(lines, line)
		end
	end

	self:drawLines(lines, 1, 1, 8)
end

return DebugOverlaySummary
