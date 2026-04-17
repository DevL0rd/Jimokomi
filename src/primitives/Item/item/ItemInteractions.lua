local WorldObject = include("src/primitives/WorldObject.lua")

local ItemInteractions = {}

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
	if self.can_consume then
		add(interactions, self.consume_action_name)
	end
	if self.can_drop then
		add(interactions, self.drop_action_name)
	end
	return interactions
end

ItemInteractions.onPickedUp = function(self, collector)
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
	if collector and collector.inventory and collector.inventory.pickupWorldItem then
		local picked_up = collector.inventory:pickupWorldItem(self)
		if picked_up <= 0 then
			return false
		end
	end
	self:onPickedUp(collector)
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

ItemInteractions.on_collision = function(self, ent, vector)
	WorldObject.on_collision(self, ent, vector)
end

return ItemInteractions
