local Behavior = include("src/classes/behaviors/Behavior.lua")

local GroundPatrolBehavior = Behavior:new({
	_type = "GroundPatrolBehavior",
	move_accel = 70,
	max_speed = 18,
	direction = -1,
	front_probe_padding = 2,
	ground_probe_padding = 4,
	randomize_on_spawn = true,

	getOwnerRadius = function(self)
		if not self.owner then
			return 0
		end
		if self.owner.getRadius then
			return self.owner:getRadius()
		end
		return 0
	end,

	isSolidAt = function(self, x, y)
		if not self.owner or not self.owner.getWorld then
			return false
		end
		local world = self.owner:getWorld()
		return world and world:isSolidAt(x, y) or false
	end,

	isOnGround = function(self)
		local radius = self:getOwnerRadius()
		return self:isSolidAt(self.owner.pos.x, self.owner.pos.y + radius + 1)
	end,

	isBlockedAhead = function(self)
		local radius = self:getOwnerRadius()
		local probe_x = self.owner.pos.x + self.direction * (radius + self.front_probe_padding)
		return self:isSolidAt(probe_x, self.owner.pos.y)
	end,

	hasGroundAhead = function(self)
		local radius = self:getOwnerRadius()
		local probe_x = self.owner.pos.x + self.direction * (radius + self.ground_probe_padding)
		local probe_y = self.owner.pos.y + radius + 2
		return self:isSolidAt(probe_x, probe_y)
	end,

	setDirection = function(self, direction)
		self.direction = direction < 0 and -1 or 1
		if self.owner then
			self.owner.direction = self.direction
		end
		self:notify("onGroundPatrolDirectionChanged", self.direction)
	end,

	reverseDirection = function(self)
		self:setDirection(-self.direction)
	end,

	afterSpawn = function(self)
		if self.randomize_on_spawn then
			self:setDirection(random_int(0, 1) == 0 and -1 or 1)
		else
			self:setDirection(self.direction)
		end
		self.owner.vel.x = 0
		self.owner.vel.y = 0
	end,

	update = function(self)
		if not self.owner then
			return
		end

		if self:isOnGround() then
			if self:isBlockedAhead() or not self:hasGroundAhead() then
				self:reverseDirection()
			end

			self.owner.vel.x += self.direction * self.move_accel * _dt
			self.owner.vel.x = mid(-self.max_speed, self.owner.vel.x, self.max_speed)
		end
	end,
})

return GroundPatrolBehavior
