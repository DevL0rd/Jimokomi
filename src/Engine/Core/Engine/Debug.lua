local EngineDebug = {}

EngineDebug.debug_state = function(self)
	if not self.debug then
		return
	end

	local state = {
		layers = {},
		master_camera = nil,
		layer_size = { w = self.w, h = self.h }
	}

	for layer_id, layer in pairs(self.layers) do
		state.layers[layer_id] = {
			entity_count = #layer.entities,
			running = layer.running,
			physics_enabled = layer.physics_enabled,
			map_id = layer.map_id
		}
	end

	if self.master_camera then
		state.master_camera = {
			pos = { x = self.master_camera.pos.x, y = self.master_camera.pos.y },
			has_target = self.master_camera.target ~= nil,
			shake_intensity = self.master_camera.shake.intensity
		}
	end

	self:appendDebug("Engine State:")
	self:appendDebug(state)
end

EngineDebug.debug_layer = function(self, layer_id)
	if not self.debug then
		return
	end

	local layer = self.layers[layer_id]
	if not layer then
		self:appendDebug("Layer " .. layer_id .. " not found")
		return
	end

	local layer_info = {
		layer_id = layer_id,
		entity_count = #layer.entities,
		running = layer.running,
		physics_enabled = layer.physics_enabled,
		map_id = layer.map_id,
		camera_pos = { x = layer.camera.pos.x, y = layer.camera.pos.y },
		entities = {}
	}

	for i = 1, min(5, #layer.entities) do
		local entity = layer.entities[i]
		layer_info.entities[i] = {
			type = entity._type or "unknown",
			pos = entity.pos and { x = entity.pos.x, y = entity.pos.y } or nil
		}
	end

	if #layer.entities > 5 then
		layer_info.entities["..."] = "(" .. (#layer.entities - 5) .. " more entities)"
	end

	self:appendDebug("Layer " .. layer_id .. ":")
	self:appendDebug(layer_info)
end

return EngineDebug
