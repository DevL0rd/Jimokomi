local Item = include("src/primitives/Item/Item.lua")

local Consumable = {}

Consumable.mixin = function(self)
	if self._consumable_mixin_applied then
		return
	end

	self._consumable_mixin_applied = true

	for key, value in pairs(Consumable) do
		if key ~= "mixin" and key ~= "init" and type(value) == "function" and self[key] == nil then
			self[key] = value
		end
	end
end

Consumable.init = function(self)
	Consumable.mixin(self)
	Item.init(self)
	self.can_consume = true
	self.consume_action_name = self.consume_action_name or "consume"
	self.is_spawned = self.spawn_rule == nil
end

Consumable.getWorld = function(self)
	if not self.layer then
		return nil
	end
	return self.layer.world
end

Consumable.getPlayer = function(self)
	if not self.layer or not self.layer.getPlayer then
		return nil
	end
	return self.layer:getPlayer()
end

Consumable.getSpawnRadius = function(self)
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

Consumable.getConsumeRadius = function(self)
	if self.consume_radius then
		return self.consume_radius
	end
	if self.getRadius then
		return self:getRadius()
	end
	if self.getWidth and self.getHeight then
		return max(self:getWidth(), self:getHeight()) * 0.5
	end
	return 0
end

Consumable.setSpawnPosition = function(self, pos)
	self.pos.x = pos.x
	self.pos.y = pos.y
	self.vel.x = 0
	self.vel.y = 0
end

Consumable.afterSpawn = function(self, spawn_pos, require_offscreen)
end

Consumable.onConsumed = function(self, consumer)
end

Consumable.onUsed = function(self, user)
	self:consume(user)
end

Consumable.respawn = function(self, require_offscreen)
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
	self:afterSpawn(spawn_pos, require_offscreen)
	return true
end

Consumable.trySpawn = function(self, require_offscreen)
	if not self.layer then
		return false
	end
	return self:respawn(require_offscreen)
end

Consumable.isConsumedByPlayer = function(self)
	local player = self:getPlayer()
	if not player then
		return false
	end

	local dx = player.pos.x - self.pos.x
	local dy = player.pos.y - self.pos.y
	local player_radius = player.getRadius and player:getRadius() or player.r or 0
	local consume_radius = player_radius + self:getConsumeRadius() + (self.consume_radius_padding or 0)
	return dx * dx + dy * dy <= consume_radius * consume_radius
end

Consumable.consume = function(self, consumer)
	if not self.can_consume then
		return false
	end
	self:onConsumed(consumer)
	return true
end

Consumable.update = function(self)
	if Item.update(self) == false then
		return false
	end

	if self.spawn_rule and not self.is_spawned then
		if not self:trySpawn(false) then
			return false
		end
	end

	if self:isConsumedByPlayer() then
		local player = self:getPlayer()
		if self:trySpawn(true) then
			self:consume(player)
		end
		return false
	end

	return true
end

Consumable.playVisual = function(self, state, overrides)
	return Item.playVisual(self, state, overrides)
end

Consumable.clearVisual = function(self)
	Item.clearVisual(self)
end

Consumable.on_collision = function(self, ent, vector)
	Item.on_collision(self, ent, vector)
end

return Consumable
