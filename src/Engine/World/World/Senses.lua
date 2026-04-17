local WorldSenses = {}

WorldSenses.getFrameId = function(self)
	return flr(time() * 60)
end

WorldSenses.resetPathBudget = function(self)
	self.path_budget_frame = self:getFrameId()
	self.path_budget_remaining = self.path_rebuilds_per_frame or 2
end

WorldSenses.resetPerceptionBudget = function(self)
	self.perception_budget_frame = self:getFrameId()
	self.perception_budget_remaining = self.perception_checks_per_frame or 3
end

WorldSenses.consumePathBudget = function(self)
	local frame_id = self:getFrameId()
	if self.path_budget_frame ~= frame_id then
		self:resetPathBudget()
	end
	if (self.path_budget_remaining or 0) <= 0 then
		return false
	end
	self.path_budget_remaining -= 1
	return true
end

WorldSenses.consumePerceptionBudget = function(self)
	local frame_id = self:getFrameId()
	if self.perception_budget_frame ~= frame_id then
		self:resetPerceptionBudget()
	end
	if (self.perception_budget_remaining or 0) <= 0 then
		return false
	end
	self.perception_budget_remaining -= 1
	return true
end

WorldSenses.hasLineOfSight = function(self, from_pos, to_pos)
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	local scope = profiler and profiler:start("world.senses.line_of_sight") or nil
	if not from_pos or not to_pos then
		if profiler then profiler:stop(scope) end
		return false
	end
	if not self.raycast or not self.layer or not self.layer.map_id then
		if profiler then profiler:stop(scope) end
		return true
	end
	local dx = to_pos.x - from_pos.x
	local dy = to_pos.y - from_pos.y
	local dist = sqrt(dx * dx + dy * dy)
	if dist <= 0 then
		if profiler then profiler:stop(scope) end
		return true
	end
	local ray = {
		pos = from_pos,
		vec = { x = dx, y = dy },
		length = dist,
	}
	local tile_t = self.raycast:rayToTiles(ray, self.layer.map_id, self.layer.tile_size or 16)
	if profiler then profiler:stop(scope) end
	return tile_t == false or tile_t >= dist
end

WorldSenses.pruneSounds = function(self, max_age_ms)
	self.recent_sounds = self.recent_sounds or {}
	if #self.recent_sounds == 0 then
		return
	end
	local frame_id = self:getFrameId()
	if self.sound_prune_frame == frame_id then
		return
	end
	self.sound_prune_frame = frame_id
	local age_limit = max_age_ms or self.sound_memory_ms or 1200
	local now = time()
	for i = #self.recent_sounds, 1, -1 do
		local sound = self.recent_sounds[i]
		if not sound or (now - sound.created_at) * 1000 > age_limit then
			deli(self.recent_sounds, i)
		end
	end
end

WorldSenses.emitSound = function(self, source, sound_type, radius, pos)
	if not source or not pos or radius == nil or radius <= 0 then
		return nil
	end
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	self.recent_sounds = self.recent_sounds or {}
	self:pruneSounds()
	local sound = {
		source = source,
		type = sound_type or "idle",
		radius = radius,
		pos = { x = pos.x, y = pos.y },
		created_at = time(),
	}
	add(self.recent_sounds, sound)
	if profiler then
		profiler:addCounter("world.senses.sounds_emitted", 1)
		profiler:observe("world.senses.sound_queue_size", #self.recent_sounds)
		profiler:observe("world.senses.sound_radius", radius)
	end
	return sound
end

WorldSenses.findNearestSound = function(self, listener, options)
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	local scope = profiler and profiler:start("world.senses.find_nearest_sound") or nil
	self:pruneSounds()
	if not self.recent_sounds or #self.recent_sounds == 0 then
		if profiler then
			profiler:observe("world.senses.sound_candidates", 0)
			profiler:stop(scope)
		end
		return nil
	end
	local listener_pos = listener and listener.pos or options and options.pos or nil
	if not listener_pos then
		if profiler then profiler:stop(scope) end
		return nil
	end
	local filter = options and options.filter or nil
	local max_listener_radius = options and options.radius or nil
	local closest = nil
	local closest_dist2 = nil
	local considered_count = 0
	for i = 1, #self.recent_sounds do
		local sound = self.recent_sounds[i]
		if sound and sound.source ~= listener and (not filter or filter(sound) == true) then
			considered_count += 1
			local dx = sound.pos.x - listener_pos.x
			local dy = sound.pos.y - listener_pos.y
			local dist2 = dx * dx + dy * dy
			local radius = sound.radius or 0
			if max_listener_radius ~= nil then
				radius = min(radius, max_listener_radius)
			end
			if dist2 <= radius * radius and (closest_dist2 == nil or dist2 < closest_dist2) then
				closest = sound
				closest_dist2 = dist2
			end
		end
	end
	if profiler then
		profiler:observe("world.senses.sound_candidates", considered_count)
		profiler:stop(scope)
	end
	return closest
end

return WorldSenses
