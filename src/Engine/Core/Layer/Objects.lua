local LayerObjects = {}

LayerObjects.add = function(layer, ent)
	add(layer.entities, ent)
	if ent.inherit_layer_debug ~= false then
		if rawget(ent, "debug") == nil then
			ent.debug = layer.debug
		end
		if rawget(ent, "debug_collision_interest") == nil then
			ent.debug_collision_interest = layer.debug
		end
	end
	ent.layer = layer
	layer:registerEntityBuckets(ent)
	ent:didAddToLayer(layer)
	layer:emit("layer.object_added", {
		layer = layer,
		object = ent,
	})
end

LayerObjects.clear = function(layer)
	for index = #layer.entities, 1, -1 do
		local ent = layer.entities[index]
		ent._delete = true
		ent:willRemoveFromLayer(layer)
		ent:unInit()
		ent:didRemoveFromLayer(layer)
		layer:unregisterEntityBuckets(ent)
		del(layer.entities, ent)
	end
end

return LayerObjects
