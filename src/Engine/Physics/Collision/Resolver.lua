local CollisionResolver = {}

local function get_collider(entity)
	return entity and entity.getCollider and entity:getCollider() or nil
end

local function resolve_velocity_against_push(self, entity, collision)
	local vel = entity and entity.vel or nil
	local push = collision and collision.vector or nil
	if not vel or not push then
		return
	end
	local push_x = push.x or 0
	local push_y = push.y or 0
	local push_len_sq = push_x * push_x + push_y * push_y
	if push_len_sq <= 0 then
		return
	end
	local push_len = sqrt(push_len_sq)
	local nx = push_x / push_len
	local ny = push_y / push_len
	local normal_velocity = vel.x * nx + vel.y * ny
	if normal_velocity < 0 then
		vel.x -= nx * normal_velocity
		vel.y -= ny * normal_velocity
	end

	local collider = get_collider(entity)
	if not collider or not collider:isDynamicBody() then
		return
	end

	local tx = -ny
	local ty = nx
	local tangent_speed = vel.x * tx + vel.y * ty
	local friction = max(collider:getFriction(), self and self.wall_friction or 0)
	if friction <= 0 then
		return
	end

	if collider:isCircle() and collider:getInverseInertia() > 0 and entity.canRotatePhysics and entity:canRotatePhysics() then
		local radius = max(0.0001, collider:getRadius())
		local inv_mass = collider:getInverseMass()
		local inv_inertia = collider:getInverseInertia()
		local contact_spin_speed = (entity.angular_vel or 0) * radius
		local slip_speed = tangent_speed - contact_spin_speed
		local tangent_mass = inv_mass + (radius * radius * inv_inertia)
		if tangent_mass > 0 then
			local friction_limit = abs(normal_velocity) * friction / max(inv_mass, 0.0001)
			local jt = -slip_speed / tangent_mass
			if friction_limit > 0 then
				jt = mid(-friction_limit, jt, friction_limit)
			end
			vel.x += tx * jt * inv_mass
			vel.y += ty * jt * inv_mass
			entity.angular_vel = (entity.angular_vel or 0) - (jt * radius * inv_inertia)
		end
		return
	end

	local tangent_drag = min(1, friction * 0.1)
	vel.x -= tx * tangent_speed * tangent_drag
	vel.y -= ty * tangent_speed * tangent_drag
end

local function apply_impulse(entity, impulse_x, impulse_y, scale)
	if not entity or not entity.vel then
		return
	end
	entity.vel.x += impulse_x * scale
	entity.vel.y += impulse_y * scale
end

local function cross2(x1, y1, x2, y2)
	return x1 * y2 - y1 * x2
end

local function get_contact_offsets(pair, entity_a, entity_b)
	local contact_x = pair and pair.contact_x or ((entity_a.pos.x + entity_b.pos.x) * 0.5)
	local contact_y = pair and pair.contact_y or ((entity_a.pos.y + entity_b.pos.y) * 0.5)
	return
		contact_x - (entity_a.pos.x or 0),
		contact_y - (entity_a.pos.y or 0),
		contact_x - (entity_b.pos.x or 0),
		contact_y - (entity_b.pos.y or 0)
end

local function apply_contact_impulse(entity, impulse_x, impulse_y, inv_mass, contact_rx, contact_ry, inv_inertia)
	apply_impulse(entity, impulse_x, impulse_y, inv_mass)
	if entity then
		entity.angular_vel = (entity.angular_vel or 0) + cross2(contact_rx or 0, contact_ry or 0, impulse_x or 0, impulse_y or 0) * (inv_inertia or 0)
	end
end

local function apply_cached_contact_impulses(pair, inv_mass_a, inv_mass_b)
	local nx = pair.normal_x or 0
	local ny = pair.normal_y or 0
	local normal_impulse = pair.normal_impulse or 0
	local tangent_impulse = pair.tangent_impulse or 0
	local tx = -ny
	local ty = nx
	if normal_impulse == 0 and tangent_impulse == 0 then
		return
	end
	local impulse_x = nx * normal_impulse + tx * tangent_impulse
	local impulse_y = ny * normal_impulse + ty * tangent_impulse
	local collider_a = get_collider(pair.entity_a)
	local collider_b = get_collider(pair.entity_b)
	local inv_inertia_a = collider_a and collider_a:getInverseInertia() or 0
	local inv_inertia_b = collider_b and collider_b:getInverseInertia() or 0
	if inv_inertia_a == 0 and inv_inertia_b == 0 then
		apply_impulse(pair.entity_a, impulse_x, impulse_y, inv_mass_a)
		apply_impulse(pair.entity_b, -impulse_x, -impulse_y, inv_mass_b)
		return
	end
	local ra_x, ra_y, rb_x, rb_y = get_contact_offsets(pair, pair.entity_a, pair.entity_b)
	apply_contact_impulse(pair.entity_a, impulse_x, impulse_y, inv_mass_a, ra_x, ra_y, inv_inertia_a)
	apply_contact_impulse(pair.entity_b, -impulse_x, -impulse_y, inv_mass_b, rb_x, rb_y, inv_inertia_b)
end

local function solve_contact_pair(pair)
	local entity_a = pair and pair.entity_a or nil
	local entity_b = pair and pair.entity_b or nil
	if not entity_a or not entity_b or entity_a._delete or entity_b._delete then
		return
	end
	local collider_a = get_collider(entity_a)
	local collider_b = get_collider(entity_b)
	if not collider_a or not collider_b then
		return
	end
	local inv_mass_a = collider_a:getInverseMass()
	local inv_mass_b = collider_b:getInverseMass()
	local inv_inertia_a = collider_a:getInverseInertia()
	local inv_inertia_b = collider_b:getInverseInertia()
	local linear_only = inv_inertia_a == 0 and inv_inertia_b == 0

	local nx = pair.normal_x or 0
	local ny = pair.normal_y or 0
	if nx == 0 and ny == 0 then
		return
	end

	local ra_x, ra_y, rb_x, rb_y = 0, 0, 0, 0
	if not linear_only then
		ra_x, ra_y, rb_x, rb_y = get_contact_offsets(pair, entity_a, entity_b)
	end
	local vel_a = entity_a.vel or { x = 0, y = 0 }
	local vel_b = entity_b.vel or { x = 0, y = 0 }
	local vel_ax = vel_a.x + (linear_only and 0 or (-(entity_a.angular_vel or 0) * ra_y))
	local vel_ay = vel_a.y + (linear_only and 0 or ((entity_a.angular_vel or 0) * ra_x))
	local vel_bx = vel_b.x + (linear_only and 0 or (-(entity_b.angular_vel or 0) * rb_y))
	local vel_by = vel_b.y + (linear_only and 0 or ((entity_b.angular_vel or 0) * rb_x))
	local rv_x = vel_ax - vel_bx
	local rv_y = vel_ay - vel_by
	local vel_along_normal = rv_x * nx + rv_y * ny
	if vel_along_normal > 0 then
		return
	end

	local ra_cross_n = linear_only and 0 or cross2(ra_x, ra_y, nx, ny)
	local rb_cross_n = linear_only and 0 or cross2(rb_x, rb_y, nx, ny)
	local normal_mass = inv_mass_a + inv_mass_b +
		(ra_cross_n * ra_cross_n) * inv_inertia_a +
		(rb_cross_n * rb_cross_n) * inv_inertia_b
	if normal_mass <= 0 then
		return
	end

	local restitution = max(collider_a:getRestitution(), collider_b:getRestitution())
	local impulse_mag = -(1 + restitution) * vel_along_normal / normal_mass
	if impulse_mag > 0 then
		local previous_normal_impulse = pair.normal_impulse or 0
		pair.normal_impulse = max(previous_normal_impulse + impulse_mag, 0)
		local applied_normal_impulse = pair.normal_impulse - previous_normal_impulse
		local impulse_x = nx * applied_normal_impulse
		local impulse_y = ny * applied_normal_impulse
		if linear_only then
			apply_impulse(entity_a, impulse_x, impulse_y, inv_mass_a)
			apply_impulse(entity_b, -impulse_x, -impulse_y, inv_mass_b)
		else
			apply_contact_impulse(entity_a, impulse_x, impulse_y, inv_mass_a, ra_x, ra_y, inv_inertia_a)
			apply_contact_impulse(entity_b, -impulse_x, -impulse_y, inv_mass_b, rb_x, rb_y, inv_inertia_b)
		end
	end

	vel_a = entity_a.vel or { x = 0, y = 0 }
	vel_b = entity_b.vel or { x = 0, y = 0 }
	vel_ax = vel_a.x + (linear_only and 0 or (-(entity_a.angular_vel or 0) * ra_y))
	vel_ay = vel_a.y + (linear_only and 0 or ((entity_a.angular_vel or 0) * ra_x))
	vel_bx = vel_b.x + (linear_only and 0 or (-(entity_b.angular_vel or 0) * rb_y))
	vel_by = vel_b.y + (linear_only and 0 or ((entity_b.angular_vel or 0) * rb_x))
	rv_x = vel_ax - vel_bx
	rv_y = vel_ay - vel_by
	vel_along_normal = rv_x * nx + rv_y * ny
	local tv_x = rv_x - vel_along_normal * nx
	local tv_y = rv_y - vel_along_normal * ny
	local tangent_len = sqrt(tv_x * tv_x + tv_y * tv_y)
	if tangent_len > 0.0001 then
		local tx = tv_x / tangent_len
		local ty = tv_y / tangent_len
		local tangent_speed = rv_x * tx + rv_y * ty
		local ra_cross_t = linear_only and 0 or cross2(ra_x, ra_y, tx, ty)
		local rb_cross_t = linear_only and 0 or cross2(rb_x, rb_y, tx, ty)
		local tangent_mass = inv_mass_a + inv_mass_b +
			(ra_cross_t * ra_cross_t) * inv_inertia_a +
			(rb_cross_t * rb_cross_t) * inv_inertia_b
		if tangent_mass <= 0 then
			return
		end
		local friction_impulse_mag = -tangent_speed / tangent_mass
		local friction = sqrt(max(0, collider_a:getFriction() * collider_b:getFriction()))
		local max_friction_impulse = (pair.normal_impulse or impulse_mag) * friction
		if max_friction_impulse > 0 then
			local previous_tangent_impulse = pair.tangent_impulse or 0
			local next_tangent_impulse = mid(-max_friction_impulse, previous_tangent_impulse + friction_impulse_mag, max_friction_impulse)
			local applied_tangent_impulse = next_tangent_impulse - previous_tangent_impulse
			pair.tangent_impulse = next_tangent_impulse
			local friction_x = tx * applied_tangent_impulse
			local friction_y = ty * applied_tangent_impulse
			if linear_only then
				apply_impulse(entity_a, friction_x, friction_y, inv_mass_a)
				apply_impulse(entity_b, -friction_x, -friction_y, inv_mass_b)
			else
				apply_contact_impulse(entity_a, friction_x, friction_y, inv_mass_a, ra_x, ra_y, inv_inertia_a)
				apply_contact_impulse(entity_b, -friction_x, -friction_y, inv_mass_b, rb_x, rb_y, inv_inertia_b)
			end
		end
	end
end

local function correct_contact_pair_position(pair)
	local entity_a = pair and pair.entity_a or nil
	local entity_b = pair and pair.entity_b or nil
	if not entity_a or not entity_b or entity_a._delete or entity_b._delete then
		return
	end
	local collider_a = get_collider(entity_a)
	local collider_b = get_collider(entity_b)
	if not collider_a or not collider_b then
		return
	end
	local inv_mass_a = collider_a:getInverseMass()
	local inv_mass_b = collider_b:getInverseMass()
	local inv_mass_total = inv_mass_a + inv_mass_b
	if inv_mass_total <= 0 then
		return
	end
	local nx = pair.normal_x or 0
	local ny = pair.normal_y or 0
	local penetration = pair.penetration or 0
	if penetration <= 0 or (nx == 0 and ny == 0) then
		return
	end

	local correction_percent = 0.2
	local correction_slop = 0.1
	local max_correction = 1.5
	local correction_mag = min(max(penetration - correction_slop, 0) * correction_percent, max_correction) / inv_mass_total
	if correction_mag <= 0 then
		return
	end

	local correction_x = nx * correction_mag
	local correction_y = ny * correction_mag
	entity_a.pos.x += correction_x * inv_mass_a
	entity_a.pos.y += correction_y * inv_mass_a
	entity_b.pos.x -= correction_x * inv_mass_b
	entity_b.pos.y -= correction_y * inv_mass_b
	entity_a._moved_this_frame = true
	entity_b._moved_this_frame = true
end

CollisionResolver.new = function(config)
	local self = config or {}
	self.collision_passes = self.collision_passes or 1
	self.wall_friction = self.wall_friction or 0.8
	self.solver_iterations = self.solver_iterations or 6
	return setmetatable(self, { __index = CollisionResolver })
end

CollisionResolver.processCollisions = function(self, entities)
	local profiler = self.profiler
	local contacts_processed = 0
	for index = 1, #entities do
		local entity = entities[index]
		if entity.ignore_physics or entity.ignore_collisions or not entity.collisions or #entity.collisions <= 0 then
			goto skip
		end
		if entity.wakePhysics then
			entity:wakePhysics()
		end

		for pass = 1, 2 do
			for collision_index = 1, #entity.collisions do
				local collision = entity.collisions[collision_index]
				local object = collision.object
				if (pass == 1 and object == "map") or
					(pass == 2 and object == "layer") or
					false then
					if collision.vector.x ~= 0 or collision.vector.y ~= 0 then
						entity._moved_this_frame = true
					end
					entity.pos:add(collision.vector)
					resolve_velocity_against_push(self, entity, collision)
					contacts_processed += 1
					if profiler then
						profiler:addCounter("collision.resolver.contacts_processed", 1)
					end
				end
			end
		end

		::skip::
	end
	return contacts_processed
end

CollisionResolver.processContactPairs = function(self, contact_pairs)
	for i = 1, #(contact_pairs or {}) do
		local pair = contact_pairs[i]
		local entity_a = pair and pair.entity_a or nil
		local entity_b = pair and pair.entity_b or nil
		if entity_a and entity_a.isPhysicsSleeping and entity_a:isPhysicsSleeping() and entity_a.wakePhysics then
			entity_a:wakePhysics()
		end
		if entity_b and entity_b.isPhysicsSleeping and entity_b:isPhysicsSleeping() and entity_b.wakePhysics then
			entity_b:wakePhysics()
		end
	end
	for i = 1, #(contact_pairs or {}) do
		correct_contact_pair_position(contact_pairs[i])
	end
	for i = 1, #(contact_pairs or {}) do
		local pair = contact_pairs[i]
		local collider_a = get_collider(pair and pair.entity_a or nil)
		local collider_b = get_collider(pair and pair.entity_b or nil)
		if collider_a and collider_b then
			apply_cached_contact_impulses(pair, collider_a:getInverseMass(), collider_b:getInverseMass())
		end
	end
	for _ = 1, self.solver_iterations or 6 do
		for i = 1, #(contact_pairs or {}) do
			solve_contact_pair(contact_pairs[i])
		end
	end
end

CollisionResolver.resolveCollisions = function(self, entities, map_id, broadphase, tile_queries)
	for _ = 1, self.collision_passes do
		broadphase:collectCollisions(entities, map_id, tile_queries)
		self:processContactPairs(broadphase and broadphase.contact_pairs or nil)
		if self:processCollisions(entities) <= 0 then
			if not broadphase or not broadphase.contact_pairs or #(broadphase.contact_pairs or {}) <= 0 then
				break
			end
		end
	end
end

return CollisionResolver
