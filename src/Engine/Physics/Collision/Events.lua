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

local function ensure_contact_state(entity, field_name)
	local state = entity[field_name]
	if not state then
		state = {}
		entity[field_name] = state
	end
	return state
end

local function clear_contact_state(entity, field_name)
	entity[field_name] = nil
end

local function process_contacts(profiler, entity, contacts, state_field, stamp, wants_enter, wants_stay, wants_exit, on_enter, on_fallback, on_stay, on_exit, counter_prefix)
	local state = entity[state_field]
	local has_current_interest = wants_enter or wants_stay or wants_exit
	if not has_current_interest and state == nil then
		return
	end

	state = ensure_contact_state(entity, state_field)
	for i = 1, #(contacts or {}) do
		local contact = contacts[i]
		local key = build_contact_key(contact)
		local previous = state[key]
		local is_new = previous == nil
		if is_new then
			previous = {}
			state[key] = previous
		end
		previous.object = contact.object
		previous.vector = contact.vector
		previous.stamp = stamp

		if is_new and wants_enter then
			if on_enter then
				if profiler then
					profiler:addCounter("collision.events." .. counter_prefix .. "_enter_callbacks", 1)
				end
				on_enter(entity, contact.object, contact.vector)
			elseif on_fallback then
				if profiler then
					profiler:addCounter("collision.events." .. counter_prefix .. "_callbacks", 1)
				end
				on_fallback(entity, contact.object, contact.vector)
			end
		elseif not is_new and wants_stay and on_stay then
			if profiler then
				profiler:addCounter("collision.events." .. counter_prefix .. "_stay_callbacks", 1)
			end
			on_stay(entity, contact.object, contact.vector)
		end
	end

	if wants_exit or not has_current_interest then
		for key, previous in pairs(state) do
			if previous.stamp ~= stamp then
				if wants_exit and on_exit then
					if profiler then
						profiler:addCounter("collision.events." .. counter_prefix .. "_exit_callbacks", 1)
					end
					on_exit(entity, previous.object, previous.vector)
				end
				state[key] = nil
			end
		end
	end

	if not next(state) then
		clear_contact_state(entity, state_field)
	end
end

CollisionEvents.handleCollisionEvents = function(self, entities)
	local profiler = self and self.profiler or nil
	self.event_stamp = (self.event_stamp or 0) + 1
	local stamp = self.event_stamp
	for index = 1, #entities do
		local entity = entities[index]
		local wants_collision_enter = resolve_interest(entity, "hasCollisionEnterInterest", "on_collision_enter")
		local wants_collision_stay = resolve_interest(entity, "hasCollisionStayInterest", "on_collision_stay")
		local wants_collision_exit = resolve_interest(entity, "hasCollisionExitInterest", "on_collision_exit")
		process_contacts(
			profiler,
			entity,
			entity.collisions,
			"_collision_contact_state",
			stamp,
			wants_collision_enter,
			wants_collision_stay,
			wants_collision_exit,
			entity.on_collision_enter,
			entity.on_collision,
			entity.on_collision_stay,
			entity.on_collision_exit,
			"collision"
		)

		local wants_overlap_enter = resolve_interest(entity, "hasOverlapEnterInterest", "on_overlap_enter")
		local wants_overlap_stay = resolve_interest(entity, "hasOverlapStayInterest", "on_overlap_stay")
		local wants_overlap_exit = resolve_interest(entity, "hasOverlapExitInterest", "on_overlap_exit")
		process_contacts(
			profiler,
			entity,
			entity.overlaps,
			"_overlap_contact_state",
			stamp,
			wants_overlap_enter,
			wants_overlap_stay,
			wants_overlap_exit,
			entity.on_overlap_enter,
			entity.on_overlap,
			entity.on_overlap_stay,
			entity.on_overlap_exit,
			"overlap"
		)
	end
end

return CollisionEvents
