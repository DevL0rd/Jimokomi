local Ecology = include("src/Game/Ecology/Ecology.lua")
local Timer = include("src/Engine/Core/Timer.lua")

local AgentPerception = {}

local function get_facing_vector(self)
	local direction = self.direction
	if direction == 2 then
		return -1, 0
	end
	if direction == 3 then
		return 1, 0
	end
	if direction == 0 then
		return 0, -1
	end
	if direction == 1 then
		return 0, 1
	end
	if direction == -1 then
		return -1, 0
	end
	if direction == 1 then
		return 1, 0
	end
	if self.vel then
		if abs(self.vel.x) > abs(self.vel.y) and abs(self.vel.x) > 1 then
			return self.vel.x < 0 and -1 or 1, 0
		end
		if abs(self.vel.y) > 1 then
			return 0, self.vel.y < 0 and -1 or 1
		end
	end
	return 1, 0
end

AgentPerception.getFacingVector = function(self)
	local fx, fy = get_facing_vector(self)
	return fx, fy
end

AgentPerception.initPerception = function(self)
	self.perception_enabled = self.perception_enabled ~= false
	self.vision_range = self.vision_range or 96
	self.vision_fov_dot = self.vision_fov_dot or 0.2
	self.hearing_range = self.hearing_range or 96
	self.perception_interval_ms = self.perception_interval_ms or 500
	self.perception_memory_ms = self.perception_memory_ms or 1200
	self.sound_update_interval_ms = self.sound_update_interval_ms or 500
	self.sound_idle_radius = self.sound_idle_radius or 18
	self.sound_move_radius = self.sound_move_radius or 52
	self.sound_land_radius = self.sound_land_radius or 72
	self.sound_idle_interval_ms = self.sound_idle_interval_ms or 1800
	self.sound_move_interval_ms = self.sound_move_interval_ms or 500
	self.perception = {
		seen_target = nil,
		heard_target = nil,
		heard_sound = nil,
		last_seen_target = nil,
		last_seen_at = nil,
		last_heard_target = nil,
		last_heard_at = nil,
	}
	self.perception_timer = self.perception_timer or Timer:new()
	self.sound_update_timer = self.sound_update_timer or Timer:new()
	self.idle_sound_timer = self.idle_sound_timer or Timer:new()
	self.move_sound_timer = self.move_sound_timer or Timer:new()
	self.perception_timer.start_time -= random_int(0, self.perception_interval_ms) / 1000
	self.sound_update_timer.start_time -= random_int(0, self.sound_update_interval_ms) / 1000
	self.idle_sound_timer.start_time -= random_int(0, self.sound_idle_interval_ms) / 1000
	self.move_sound_timer.start_time -= random_int(0, self.sound_move_interval_ms) / 1000
	self.was_grounded_for_sound = self.was_grounded_for_sound or false
end

AgentPerception.canPerceiveFaction = function(self, target)
	if not target or target == self or target._delete then
		return false
	end
	if not target.getFaction and not target.faction then
		return false
	end
	local target_faction = target.getFaction and target:getFaction() or target.faction
	if target_faction == self:getFaction() then
		return false
	end
	return true
end

AgentPerception.canSeeTarget = function(self, target)
	if not target or not target.pos then
		return false
	end
	local world = self:getWorld()
	if not world then
		return false
	end
	local dx = target.pos.x - self.pos.x
	local dy = target.pos.y - self.pos.y
	local dist2 = dx * dx + dy * dy
	if dist2 > self.vision_range * self.vision_range then
		return false
	end
	local dist = sqrt(dist2)
	if dist > 0 then
		local fx, fy = get_facing_vector(self)
		local dot = (dx / dist) * fx + (dy / dist) * fy
		if dot < self.vision_fov_dot then
			return false
		end
	end
	return world:hasLineOfSight(self.pos, target.pos)
end

AgentPerception.getSeenTarget = function(self)
	if not self.layer then
		return nil
	end
	local own_faction = self.getFaction and self:getFaction() or self.faction
	local closest = nil
	local closest_dist2 = nil
	local buckets = self.layer.entities_by_faction

	local function consider_bucket(bucket)
		for i = 1, #(bucket or {}) do
			local candidate = bucket[i]
			if self:canPerceiveFaction(candidate) and self:canSeeTarget(candidate) then
				local dx = candidate.pos.x - self.pos.x
				local dy = candidate.pos.y - self.pos.y
				local dist2 = dx * dx + dy * dy
				if closest_dist2 == nil or dist2 < closest_dist2 then
					closest = candidate
					closest_dist2 = dist2
				end
			end
		end
	end

	if buckets then
		for faction, bucket in pairs(buckets) do
			if faction ~= own_faction then
				consider_bucket(bucket)
			end
		end
	else
		consider_bucket(self.layer.entities)
	end

	return closest
end

AgentPerception.getHeardSound = function(self)
	local world = self:getWorld()
	if not world then
		return nil
	end
	return world:findNearestSound(self, {
		radius = self.hearing_range,
		filter = function(sound)
			local source = sound.source
			if not source or source == self or source._delete then
				return false
			end
			if not self:canPerceiveFaction(source) then
				return false
			end
			local max_radius = min(self.hearing_range or 0, sound.radius or 0)
			return max_radius > 0
		end,
	})
end

AgentPerception.updatePerception = function(self)
	if not self.perception_enabled then
		return
	end
	if not self.perception_timer:hasElapsed(self.perception_interval_ms) then
		return
	end
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	local scope = profiler and profiler:start("game.agent.update_perception") or nil
	local world = self:getWorld()
	if world and not world:consumePerceptionBudget() then
		if profiler then
			profiler:addCounter("game.agent.perception_budget_denied", 1)
			profiler:stop(scope)
		end
		return
	end

	local seen_target = self:getSeenTarget()
	local heard_sound = self:getHeardSound()
	local heard_target = heard_sound and heard_sound.source or nil

	self.perception.seen_target = seen_target
	self.perception.heard_sound = heard_sound
	self.perception.heard_target = heard_target

	if seen_target then
		self.perception.last_seen_target = seen_target
		self.perception.last_seen_at = time()
	end
	if heard_target then
		self.perception.last_heard_target = heard_target
		self.perception.last_heard_at = time()
	end
	if profiler then profiler:stop(scope) end
end

AgentPerception.getPerceivedTarget = function(self)
	local perception = self.perception
	if not perception then
		return nil
	end
	if perception.seen_target and not perception.seen_target._delete then
		return perception.seen_target
	end
	if perception.heard_target and not perception.heard_target._delete then
		return perception.heard_target
	end

	local memory_ms = self.perception_memory_ms or 1200
	local now = time()
	if perception.last_seen_target and not perception.last_seen_target._delete and perception.last_seen_at and (now - perception.last_seen_at) * 1000 <= memory_ms then
		return perception.last_seen_target
	end
	if perception.last_heard_target and not perception.last_heard_target._delete and perception.last_heard_at and (now - perception.last_heard_at) * 1000 <= memory_ms then
		return perception.last_heard_target
	end
	return nil
end

AgentPerception.getPerceivedTargetForAction = function(self, action)
	local target = self:getPerceivedTarget()
	if not target then
		return nil
	end
	if self:getActionToward(target) ~= action then
		return nil
	end
	return target
end

AgentPerception.isOnGroundForSound = function(self)
	if self.isOnGround then
		return self:isOnGround()
	end
	return false
end

AgentPerception.emitSound = function(self, sound_type, radius)
	local world = self:getWorld()
	if not world or not self.pos then
		return nil
	end
	return world:emitSound(self, sound_type, radius, self.pos)
end

AgentPerception.updateSounding = function(self)
	if not self.perception_enabled then
		return
	end
	if self.sound_update_timer and not self.sound_update_timer:hasElapsed(self.sound_update_interval_ms or 120) then
		return
	end
	local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
	local scope = profiler and profiler:start("game.agent.update_sounding") or nil
	local is_grounded = self:isOnGroundForSound()
	if is_grounded and not self.was_grounded_for_sound then
		self:emitSound("land", self.sound_land_radius)
	end
	self.was_grounded_for_sound = is_grounded

	local move_speed2 = self.vel and (self.vel.x * self.vel.x + self.vel.y * self.vel.y) or 0
	local is_moving = move_speed2 > 16

	if is_moving then
		if self.move_sound_timer:hasElapsed(self.sound_move_interval_ms) then
			self:emitSound("move", self.sound_move_radius)
		end
	elseif self.idle_sound_timer:hasElapsed(self.sound_idle_interval_ms) then
		self:emitSound("idle", self.sound_idle_radius)
	end
	if profiler then profiler:stop(scope) end
end

return AgentPerception
