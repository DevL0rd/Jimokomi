local CollisionEvents = {}

local function resolve_interest(entity, method_name, fallback_handler_name)
	if entity[method_name] ~= nil then
		return entity[method_name](entity) == true
	end
	return entity[fallback_handler_name] ~= nil
end

local function get_object_key(object)
	if type(object) == "string" then
		return object
	end
	if not object then
		return "nil"
	end
	if object.object_id ~= nil then
		return "obj:" .. object.object_id
	end
	return "type:" .. tostr(object._type or object)
end

local function build_contact_key(contact)
	return get_object_key(contact.object)
end

local function build_contact_maps(contacts)
	local keys = {}
	local map = {}
	for i = 1, #(contacts or {}) do
		local contact = contacts[i]
		local key = build_contact_key(contact)
		keys[key] = true
		map[key] = contact
	end
	return keys, map
end

CollisionEvents.handleCollisionEvents = function(self, entities)
	local profiler = self and self.profiler or nil
	for index = 1, #entities do
		local entity = entities[index]
		local wants_collision_enter = resolve_interest(entity, "hasCollisionEnterInterest", "on_collision_enter")
		local wants_collision_stay = resolve_interest(entity, "hasCollisionStayInterest", "on_collision_stay")
		local wants_collision_exit = resolve_interest(entity, "hasCollisionExitInterest", "on_collision_exit")
		local current_collision_keys, current_collision_map = build_contact_maps(entity.collisions)
		local previous_collision_keys = entity._previous_collision_keys or {}
		if entity.collisions and #entity.collisions > 0 then
			for collision_index = 1, #entity.collisions do
				local collision = entity.collisions[collision_index]
				local key = build_contact_key(collision)
				local is_new = previous_collision_keys[key] ~= true
				if is_new and wants_collision_enter then
					if entity.on_collision_enter then
						if profiler then
							profiler:addCounter("collision.events.collision_enter_callbacks", 1)
						end
						entity:on_collision_enter(collision.object, collision.vector)
					elseif entity.on_collision then
						if profiler then
							profiler:addCounter("collision.events.collision_callbacks", 1)
						end
						entity:on_collision(collision.object, collision.vector)
					end
				elseif not is_new and wants_collision_stay and entity.on_collision_stay then
					if profiler then
						profiler:addCounter("collision.events.collision_stay_callbacks", 1)
					end
					entity:on_collision_stay(collision.object, collision.vector)
				end
			end
		end

		for key in pairs(previous_collision_keys) do
			if not current_collision_keys[key] and wants_collision_exit and entity.on_collision_exit then
				local previous = entity._previous_collision_map and entity._previous_collision_map[key] or nil
				if profiler then
					profiler:addCounter("collision.events.collision_exit_callbacks", 1)
				end
				entity:on_collision_exit(previous and previous.object or nil, previous and previous.vector or nil)
			end
		end
		entity._previous_collision_keys = current_collision_keys
		entity._previous_collision_map = current_collision_map

		local wants_overlap_enter = resolve_interest(entity, "hasOverlapEnterInterest", "on_overlap_enter")
		local wants_overlap_stay = resolve_interest(entity, "hasOverlapStayInterest", "on_overlap_stay")
		local wants_overlap_exit = resolve_interest(entity, "hasOverlapExitInterest", "on_overlap_exit")
		local current_overlap_keys, current_overlap_map = build_contact_maps(entity.overlaps)
		local previous_overlap_keys = entity._previous_overlap_keys or {}
		if entity.overlaps and #entity.overlaps > 0 then
			for overlap_index = 1, #entity.overlaps do
				local overlap = entity.overlaps[overlap_index]
				local key = build_contact_key(overlap)
				local is_new = previous_overlap_keys[key] ~= true
				if is_new and wants_overlap_enter then
					if entity.on_overlap_enter then
						if profiler then
							profiler:addCounter("collision.events.overlap_enter_callbacks", 1)
						end
						entity:on_overlap_enter(overlap.object, overlap.vector)
					elseif entity.on_overlap then
						if profiler then
							profiler:addCounter("collision.events.overlap_callbacks", 1)
						end
						entity:on_overlap(overlap.object, overlap.vector)
					end
				elseif not is_new and wants_overlap_stay and entity.on_overlap_stay then
					if profiler then
						profiler:addCounter("collision.events.overlap_stay_callbacks", 1)
					end
					entity:on_overlap_stay(overlap.object, overlap.vector)
				end
			end
		end

		for key in pairs(previous_overlap_keys) do
			if not current_overlap_keys[key] and wants_overlap_exit and entity.on_overlap_exit then
				local previous = entity._previous_overlap_map and entity._previous_overlap_map[key] or nil
				if profiler then
					profiler:addCounter("collision.events.overlap_exit_callbacks", 1)
				end
				entity:on_overlap_exit(previous and previous.object or nil, previous and previous.vector or nil)
			end
		end
		entity._previous_overlap_keys = current_overlap_keys
		entity._previous_overlap_map = current_overlap_map
	end
end

return CollisionEvents
