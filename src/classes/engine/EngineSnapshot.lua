local EngineSnapshot = {}

EngineSnapshot.toSnapshot = function(self)
	local layers = {}
	for layer_id, layer in pairs(self.layers) do
		layers[layer_id] = layer.toSnapshot and layer:toSnapshot() or nil
	end
	return {
		w = self.w,
		h = self.h,
		fill_color = self.fill_color,
		stroke_color = self.stroke_color,
		next_object_id = self.next_object_id,
		layers = layers,
	}
end

EngineSnapshot.applySnapshot = function(self, snapshot, registry)
	if not snapshot then
		return
	end
	self.w = snapshot.w or self.w
	self.h = snapshot.h or self.h
	self.fill_color = snapshot.fill_color or self.fill_color
	self.stroke_color = snapshot.stroke_color or self.stroke_color
	self.next_object_id = snapshot.next_object_id or self.next_object_id
	for layer_id, layer_snapshot in pairs(snapshot.layers or {}) do
		local layer = self.layers[layer_id]
		if not layer then
			layer = self:createLayer(layer_id, true, layer_snapshot and layer_snapshot.map_id or nil)
		end
		if layer.applySnapshot then
			layer:applySnapshot(layer_snapshot, registry)
		end
	end
end

return EngineSnapshot
