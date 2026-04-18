local Layer = include("src/Engine/Core/Layer.lua")
local Screen = include("src/Engine/Core/Screen.lua")

local EngineLayers = {}

EngineLayers.createLayer = function(self, layer_id, physics_enabled, map_id)
	self:ensureServices()
	if physics_enabled == nil then
		physics_enabled = true
	end

	local layer = Layer:new({
		debug = self.debug,
		debug_tile_labels = self.debug,
		layer_id = layer_id,
		physics_enabled = physics_enabled,
		gravity = self.gravity,
		map_id = map_id,
		engine = self,
		w = self.w,
		h = self.h,
		spatial_index_cell_size = self.spatial_index_cell_size or 32,
		collision_broadphase_cell_size = self.collision_broadphase_cell_size or self.spatial_index_cell_size or 32,
	})

	layer.camera:setBounds(0, self.w - Screen.w, 0, self.h - Screen.h)

	self.layers[layer_id] = layer
	add(self.sorted_layer_ids, layer_id)
	for i = #self.sorted_layer_ids - 1, 1, -1 do
		if self.sorted_layer_ids[i] <= self.sorted_layer_ids[i + 1] then
			break
		end
		local temp = self.sorted_layer_ids[i]
		self.sorted_layer_ids[i] = self.sorted_layer_ids[i + 1]
		self.sorted_layer_ids[i + 1] = temp
	end

	if not self.master_camera then
		self.master_camera = layer.camera
	end

	return layer
end

EngineLayers.getLayer = function(self, layer_id)
	return self.layers[layer_id]
end

EngineLayers.linkCameras = function(self, layer_id, parallax_x, parallax_y)
	local layer = self.layers[layer_id]
	if layer and self.master_camera then
		layer.camera:linkToCamera(self.master_camera, parallax_x, parallax_y)
	end
end

EngineLayers.setMasterCamera = function(self, layer_id)
	local layer = self.layers[layer_id]
	if layer then
		self.master_camera = layer.camera
	end
end

return EngineLayers
