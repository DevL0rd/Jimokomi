local WorldObject = include("src/Engine/Objects/WorldObject.lua")

local ItemInteractions = {}

ItemInteractions.spawnPickupReplacement = function(self)
	if self.replace_on_pickup ~= true or not self.spawn_item_path or not self.spawn_rule then
		return false
	end
	local world = self.getWorld and self:getWorld() or nil
	if not world then
		return false
	end
	local ItemClass = include(self.spawn_item_path)
	if not ItemClass then
		return false
	end
	return world:spawn(ItemClass, {
		layer = self.layer,
	}, self.spawn_rule, true, self.getPickupRadius and self:getPickupRadius() or nil) ~= nil
end

ItemInteractions.getAvailableInteractions = function(self)
	local interactions = {}
	if self.can_pickup then
		add(interactions, self.pickup_action_name)
	end
	if self.can_use then
		add(interactions, self.use_action_name)
	end
	if self.can_place then
		add(interactions, self.place_action_name)
	end
	if self.can_drop then
		add(interactions, self.drop_action_name)
	end
	return interactions
end

ItemInteractions.onPickedUp = function(self, collector, picked_up)
	picked_up = picked_up or self:getStackSize()
	self:spawnPickupReplacement()
	if picked_up >= self:getStackSize() then
		self:destroy()
	else
		self:removeFromStack(picked_up)
	end
end

ItemInteractions.onUsed = function(self, user)
end

ItemInteractions.onPlaced = function(self, placer)
end

ItemInteractions.onDropped = function(self, dropper)
end

ItemInteractions.pickup = function(self, collector)
	if not self.can_pickup then
		return false
	end
	local inventory = nil
	if collector and collector.getPrimaryInventory then
		inventory = collector:getPrimaryInventory()
	elseif collector then
		inventory = collector.inventory
	end
	local picked_up = self:getStackSize()
	if inventory and inventory.pickupWorldItem then
		picked_up = inventory:pickupWorldItem(self)
		if picked_up <= 0 then
			return false
		end
	end
	self:onPickedUp(collector, picked_up)
	return true
end

ItemInteractions.use = function(self, user)
	if not self.can_use then
		return false
	end
	self:onUsed(user)
	return true
end

ItemInteractions.place = function(self, placer)
	if not self.can_place then
		return false
	end
	self:onPlaced(placer)
	return true
end

ItemInteractions.drop = function(self, dropper)
	if not self.can_drop then
		return false
	end
	self:onDropped(dropper)
	return true
end

ItemInteractions.getPlayer = function(self)
	if not self.layer or not self.layer.getPlayer then
		return nil
	end
	return self.layer:getPlayer()
end

ItemInteractions.getPickupRadius = function(self)
	if self.pickup_radius then
		return self.pickup_radius
	end
	if self.getRadius then
		return self:getRadius()
	end
	if self.getWidth and self.getHeight then
		return max(self:getWidth(), self:getHeight()) * 0.5
	end
	return 0
end

ItemInteractions.isTouchingCollector = function(self, collector)
	if not collector then
		return false
	end
	local dx = collector.pos.x - self.pos.x
	local dy = collector.pos.y - self.pos.y
	local collector_radius = collector.getRadius and collector:getRadius() or collector.r or 0
	local pickup_radius = collector_radius + self:getPickupRadius() + (self.pickup_radius_padding or 0)
	return dx * dx + dy * dy <= pickup_radius * pickup_radius
end

ItemInteractions.tryTouchPickup = function(self)
	local collector = self:getPlayer()
	if not collector or not self:isTouchingCollector(collector) then
		return false
	end
	return self:pickup(collector)
end

ItemInteractions.on_collision = function(self, ent, vector)
	WorldObject.on_collision(self, ent, vector)
end

return ItemInteractions
