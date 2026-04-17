local Timer = include("src/Engine/Core/Timer.lua")

local AgentSpawning = {}

AgentSpawning.getSpawnRadius = function(self)
	if self.spawn_radius then
		return self.spawn_radius
	end
	if self.getRadius then
		return self:getRadius()
	end
	if self.getWidth and self.getHeight then
		return max(self:getWidth(), self:getHeight()) * 0.5
	end
	return 0
end

AgentSpawning.getEatRadius = function(self)
	if self.eat_radius then
		return self.eat_radius
	end
	if self.getRadius then
		return self:getRadius()
	end
	if self.getWidth and self.getHeight then
		return max(self:getWidth(), self:getHeight()) * 0.5
	end
	return 0
end

AgentSpawning.setSpawnPosition = function(self, pos)
	self.pos.x = pos.x
	self.pos.y = pos.y
	self.vel.x = 0
	self.vel.y = 0
end

AgentSpawning.afterSpawn = function(self, spawn_pos, require_offscreen)
end

AgentSpawning.onEaten = function(self)
end

AgentSpawning.respawn = function(self, require_offscreen)
	local world = self:getWorld()
	if not world or not self.spawn_rule then
		return false
	end

	local spawn_pos = world:resolveRule(
		self.spawn_rule,
		require_offscreen,
		self:getSpawnRadius()
	)
	if not spawn_pos then
		return false
	end

	self:setSpawnPosition(spawn_pos)
	self.is_spawned = true
	if self.afterSpawn then
		self:afterSpawn(spawn_pos, require_offscreen)
	end
	return true
end

AgentSpawning.trySpawn = function(self, require_offscreen)
	if not self.layer then
		return false
	end
	return self:respawn(require_offscreen)
end

AgentSpawning.isEatenByPlayer = function(self)
	if not self.edible then
		return false
	end

	local player = self:getPlayer()
	if not player then
		return false
	end

	local dx = player.pos.x - self.pos.x
	local dy = player.pos.y - self.pos.y
	local player_radius = player.getRadius and player:getRadius() or player.r or 0
	local eat_radius = player_radius + self:getEatRadius() + (self.eat_radius_padding or 0)
	return dx * dx + dy * dy <= eat_radius * eat_radius
end

AgentSpawning.update = function(self)
	if self.eat_check_timer == nil then
		self.eat_check_interval_ms = self.eat_check_interval_ms or 120
		self.eat_check_timer = Timer:new()
		self.eat_check_timer.start_time -= random_int(0, self.eat_check_interval_ms) / 1000
	end

	if self.spawn_rule and not self.is_spawned then
		if not self:trySpawn(false) then
			return false
		end
	end

	if self.edible and self.can_pickup ~= true and self.eat_check_timer:hasElapsed(self.eat_check_interval_ms or 120) and self:isEatenByPlayer() then
		if self:trySpawn(true) and self.onEaten then
			self:onEaten()
		end
		return false
	end

	self:updatePerception()
	self:updateSounding()
	self:updateActionPlan()

	if self.update_agent then
		self:update_agent()
	end

	return true
end

return AgentSpawning
