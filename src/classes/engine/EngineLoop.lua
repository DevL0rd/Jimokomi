local DebugOverlay = include("src/classes/DebugOverlay.lua")

local EngineLoop = {}

_dt = 0

EngineLoop.update = function(self)
	self:ensureServices()
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
		layer:update()
	end
end

EngineLoop.draw = function(self)
	cls()

	if self.master_camera then
		local cam_screen = self.master_camera:layerToScreen({ x = 0, y = 0 })

		if self.fill_color > -1 then
			rectfill(
				cam_screen.x,
				cam_screen.y,
				cam_screen.x + self.w - 1,
				cam_screen.y + self.h - 1,
				self.fill_color
			)
		end

		if self.stroke_color > -1 then
			rect(
				cam_screen.x,
				cam_screen.y,
				cam_screen.x + self.w - 1,
				cam_screen.y + self.h - 1,
				self.stroke_color
			)
		end
	end

	for i = 1, #self.sorted_layer_ids do
		local layer_id = self.sorted_layer_ids[i]
		self.layers[layer_id]:draw()
	end

	if self.debug then
		if not self.debug_overlay then
			self.debug_overlay = DebugOverlay:new()
		end
		self.debug_overlay:draw(self)
	end

	camera()
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
