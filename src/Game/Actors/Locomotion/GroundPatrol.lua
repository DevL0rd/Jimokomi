local Mode = include("src/Game/Actors/Locomotion/Mode.lua")
local Pathing = include("src/Game/Actors/Locomotion/Pathing.lua")
local GroundPath = include("src/Game/Actors/Locomotion/Pathing/Ground.lua")

local GroundPatrol = Mode:new({
	_type = "GroundPatrol",
	move_accel = 70,
	max_speed = 18,
	direction = -1,
	front_probe_padding = 2,
	ground_probe_padding = 4,
	patrol_distance = 32,
	randomize_on_spawn = true,
	patrol_target = nil,
	use_pathing_when_stuck = false,

	init = function(self)
		self.pathing = Pathing:new({
			owner = self.owner,
			policy = GroundPath,
			repath_distance = 10,
			repath_rate_hz = 2,
			repath_fail_rate_hz = 1,
		})
		self.patrol_target = self.patrol_target or nil
	end,

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
		self.patrol_target = nil
		self.pathing:invalidate()
	end,

	choosePatrolTarget = function(self)
		local owner = self.owner
		local world = self:getWorld()
		if not owner or not world then
			return nil
		end
		self.patrol_target = {
			x = owner.pos.x + self.direction * self.patrol_distance,
			y = owner.pos.y,
		}
		self.pathing:rebuild(self.patrol_target)
		return self.patrol_target
	end,

	updateAutonomous = function(self)
		if not self.owner then
			return
		end

		if self:isOnGround() then
			if self:isBlockedAhead() or not self:hasGroundAhead() then
				self:reverseDirection()
				self.patrol_target = nil
				self.pathing:invalidate()
			end

			if self.use_pathing_when_stuck then
				local target = self.patrol_target
				if not target then
					target = self:choosePatrolTarget()
				end
				local waypoint = target and self.pathing:getWaypoint(target, 10) or nil
				if waypoint then
					local dx = waypoint.x - self.owner.pos.x
					if abs(dx) <= 8 then
						target = self:choosePatrolTarget()
						waypoint = target and self.pathing:getWaypoint(target, 10) or nil
						dx = waypoint and (waypoint.x - self.owner.pos.x) or dx
					end
					if dx < 0 then
						self:setDirection(-1)
					elseif dx > 0 then
						self:setDirection(1)
					end
				end
			end

			self.owner.vel.x += self.direction * self.move_accel * _dt
			self.owner.vel.x = mid(-self.max_speed, self.owner.vel.x, self.max_speed)
		end
	end,
})

return GroundPatrol
