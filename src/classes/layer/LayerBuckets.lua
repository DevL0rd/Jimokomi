local LayerBuckets = {}

LayerBuckets.reset = function(layer)
	layer.attached_entities = {}
	layer.physics_entities = {}
	layer.collidable_entities = {}
	layer.drawable_entities = {}
end

LayerBuckets.registerEntity = function(layer, ent)
	if ent.draw and (ent.needsDraw == nil or ent:needsDraw()) then
		add(layer.drawable_entities, ent)
	end
	if ent.parent then
		add(layer.attached_entities, ent)
	end
	if not ent.ignore_physics then
		add(layer.physics_entities, ent)
	end
	if not ent.ignore_collisions then
		if ent.getCollider == nil or ent:getCollider() == nil or ent:getCollider():isEnabled() then
			add(layer.collidable_entities, ent)
		end
	end
end

LayerBuckets.unregisterEntity = function(layer, ent)
	del(layer.drawable_entities, ent)
	del(layer.attached_entities, ent)
	del(layer.physics_entities, ent)
	del(layer.collidable_entities, ent)
end

LayerBuckets.refreshAttachment = function(layer, ent)
	del(layer.attached_entities, ent)
	if ent.parent then
		add(layer.attached_entities, ent)
	end
end

return LayerBuckets
