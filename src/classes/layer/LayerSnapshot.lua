local LayerSnapshot = {}

LayerSnapshot.toSnapshot = function(layer)
	local entities = {}
	for _, ent in pairs(layer.entities) do
		if ent.toSnapshot and ent.isSnapshotEnabled and ent:isSnapshotEnabled() then
			add(entities, ent:toSnapshot())
		end
	end
	return {
		layer_id = layer.layer_id,
		map_id = layer.map_id,
		physics_enabled = layer.physics_enabled,
		player_id = layer.player and layer.player.object_id or nil,
		entities = entities,
	}
end

LayerSnapshot.applySnapshot = function(layer, snapshot, registry)
	if not snapshot then
		return
	end
	layer.map_id = snapshot.map_id
	layer.physics_enabled = snapshot.physics_enabled ~= false

	layer:clearObjects()

	local created = {}
	for _, entity_snapshot in pairs(snapshot.entities or {}) do
		local EntityClass = registry and registry[entity_snapshot._type] or nil
		if EntityClass then
			local ent = EntityClass:new({
				layer = layer,
			})
			if ent.applySnapshot then
				ent:applySnapshot(entity_snapshot)
			end
			created[ent.object_id] = ent
		end
	end

	for _, ent in pairs(layer.entities) do
		local transform_snapshot = ent.transform and ent.transform.pending_attachment_snapshot or nil
		if transform_snapshot == nil then
			goto continue
		end
		local parent_id = transform_snapshot.parent_id
		local parent = parent_id and created[parent_id] or nil
		if parent then
			ent:attachTo(parent, transform_snapshot.slot_name, {
				offset = transform_snapshot.offset,
				enabled = transform_snapshot.enabled,
				sync_position = transform_snapshot.sync_position,
				sync_when_disabled = transform_snapshot.sync_when_disabled,
				forward_collisions = transform_snapshot.forward_collisions,
			})
		end
		ent.transform.pending_attachment_snapshot = nil
		::continue::
	end

	if snapshot.player_id then
		layer.player = created[snapshot.player_id]
		if layer.player then
			layer.camera:setTarget(layer.player)
		end
	else
		layer.player = nil
	end
end

return LayerSnapshot
