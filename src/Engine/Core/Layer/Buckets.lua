local LayerBuckets = {}

LayerBuckets.reset = function(layer)
	layer.attached_entities = {}
	layer.physics_entities = {}
	layer.collidable_entities = {}
	layer.drawable_entities = {}
	layer.entities_by_faction = {}
end

local function get_entity_faction(ent)
	if not ent then
		return nil
	end
	if ent.getFaction then
		return ent:getFaction()
	end
	return ent.faction
end

local function register_faction_bucket(layer, ent)
	local faction = get_entity_faction(ent)
	if faction == nil then
		return
	end
	layer.entities_by_faction = layer.entities_by_faction or {}
	layer.entities_by_faction[faction] = layer.entities_by_faction[faction] or {}
	add(layer.entities_by_faction[faction], ent)
end

local function unregister_faction_bucket(layer, ent)
	local faction = get_entity_faction(ent)
	local bucket = faction and layer.entities_by_faction and layer.entities_by_faction[faction] or nil
	if bucket then
		del(bucket, ent)
	end
end

LayerBuckets.registerEntity = function(layer, ent)
	if ent.draw and (ent.needsDraw == nil or ent:needsDraw()) then
		add(layer.drawable_entities, ent)
	end
	register_faction_bucket(layer, ent)
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
	unregister_faction_bucket(layer, ent)
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

LayerBuckets.refreshEntity = function(layer, ent)
	LayerBuckets.unregisterEntity(layer, ent)
	LayerBuckets.registerEntity(layer, ent)
end

return LayerBuckets
