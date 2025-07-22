local Layer = include("src/classes/Layer.lua")
local Vector = include("src/classes/Vector.lua")
local Screen = include("src/classes/Screen.lua")

local last_time = 0
_dt = 0
local debug_text = ""
function print_debug(text)
	debug_text = debug_text .. text .. "\n"
end

local Engine = {
	layers = {},
	master_camera = nil,
	debug = DEBUG or false,
	w = 16 * 64,
	h = 16 * 32,
	gravity = Vector:new({
		y = 200
	}),
	fill_color = 32,
	stroke_color = 5,
	createLayer = function(self, layer_id, physics_enabled, map_id)
		physics_enabled = physics_enabled or true
		local layer = Layer:new()
		layer.debug = self.debug
		layer.layer_id = layer_id
		layer.physics_enabled = physics_enabled
		layer.gravity = self.gravity
		layer.map_id = map_id

		layer.w = self.w
		layer.h = self.h

		layer.camera:setBounds(0, self.w - Screen.w, 0, self.h - Screen.h)

		self.layers[layer_id] = layer

		if not self.master_camera then
			self.master_camera = layer.camera
		end

		return layer
	end,

	getLayer = function(self, layer_id)
		return self.layers[layer_id]
	end,

	linkCameras = function(self, layer_id, parallax_x, parallax_y)
		local layer = self.layers[layer_id]
		if layer and self.master_camera then
			layer.camera:linkToCamera(self.master_camera, parallax_x, parallax_y)
		end
	end,

	setMasterCamera = function(self, layer_id)
		local layer = self.layers[layer_id]
		if layer then
			self.master_camera = layer.camera
		end
	end,

	update = function(self)
		_dt = time() - last_time
		last_time = time()
		debug_text = ""

		if self.master_camera then
			self.master_camera:update()
		end

		for layer_id, world in pairs(self.layers) do
			if world.camera ~= self.master_camera and world.camera.parallax_factor then
				world.camera:linkToCamera(self.master_camera,
					world.camera.parallax_factor.x,
					world.camera.parallax_factor.y)
			end

			world:update()
		end
	end,

	draw = function(self)
		cls()

		if self.master_camera then
			local cam_screen = self.master_camera:worldToScreen({ x = 0, y = 0 })

			if self.fill_color > -1 then
				rectfill(cam_screen.x, cam_screen.y,
					cam_screen.x + self.w - 1,
					cam_screen.y + self.h - 1,
					self.fill_color)
			end

			if self.stroke_color > -1 then
				rect(cam_screen.x, cam_screen.y,
					cam_screen.x + self.w - 1,
					cam_screen.y + self.h - 1,
					self.stroke_color)
			end
		end

		local layer_keys = {}
		for k, v in pairs(self.layers) do
			add(layer_keys, k)
		end

		for i = 1, #layer_keys - 1 do
			for j = 1, #layer_keys - i do
				if layer_keys[j] > layer_keys[j + 1] then
					local temp = layer_keys[j]
					layer_keys[j] = layer_keys[j + 1]
					layer_keys[j + 1] = temp
				end
			end
		end

		for _, layer_id in pairs(layer_keys) do
			self.layers[layer_id]:draw()
		end
		if self.debug then
			print(debug_text, 1, 1) -- Print debug text at the top-left corner
		end
		-- Reset camera after drawing all layers
		camera()
	end,

	start = function(self)
		last_time = time()
		for _, world in pairs(self.layers) do
			world:start()
		end
	end,

	stop = function(self)
		for _, world in pairs(self.layers) do
			world:stop()
		end
	end
}

return Engine
