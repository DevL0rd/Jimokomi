local DebugOverlay = include("src/Engine/Core/DebugOverlay.lua")

local EngineLoop = {}

_dt = 0

EngineLoop.update = function(self)
	self:ensureServices()
	local update_scope = self.profiler and self.profiler:start("engine.update") or nil
	if self.clock.last_time == 0 then
		self.clock:start()
	end

	_dt = self.clock:tick()
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
	if self.profiler then
		self.profiler:stop(update_scope)
		self.profiler:reportIfDue()
	end
end

EngineLoop.draw = function(self)
	self:ensureServices()
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

	if self.debug_overlay_enabled then
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
		self.profiler:stop(draw_scope)
		local runtime_cpu = stat and stat(1) or 0
		local measured_dt = self.clock and self.clock.measured_delta or _dt or 0
		self.profiler:setRuntimeStat("fps", runtime_fps or 0)
		self.profiler:setRuntimeStat("cpu", runtime_cpu or 0)
		self.profiler:setRuntimeStat("draw_fps", runtime_fps or 0)
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
