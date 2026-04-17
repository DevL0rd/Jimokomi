local Mode = include("src/Game/Actors/Locomotion/Mode.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")

local Flight = Mode:new({
	_type = "Flight",
	state_names = {
		flying = "flying",
		landing = "landing",
		landed = "landed",
	},
	explore_speed = 32,
	landing_speed = 85,
	flight_duration_min = 4800,
	flight_duration_max = 9600,
	landed_duration_min = 2400,
	landed_duration_max = 5200,
	explore_retarget_min = 1200,
	explore_retarget_max = 3200,
	flutter_frequency = 7,
	flutter_amplitude = 8,
	flee_speed = 120,
	flee_anchor_distance = 96,
	flee_upward_clamp = -0.35,
	explore_rule = nil,
	landing_rule = nil,
	landing_radius_x = 240,
	landing_radius_y = 160,
	anchor = nil,
	explore_target = nil,
	landing_target = nil,
	facing_right = false,
	flutter_time = 0,
	explore_retarget_time = 0,

	init = function(self)
		local owner = self.owner
		local start_x = owner and owner.pos and owner.pos.x or 0
		local start_y = owner and owner.pos and owner.pos.y or 0
		self.explore_timer = Timer:new()
		self.anchor = Vector:new({ x = start_x, y = start_y })
		self.explore_target = Vector:new({ x = start_x, y = start_y })
	end,

	getOwnerRadius = function(self)
		if not self.owner then
			return 0
		end
		if self.owner.getSpawnRadius then
			return self.owner:getSpawnRadius()
		end
		if self.owner.getRadius then
			return self.owner:getRadius()
		end
		if self.owner.getWidth and self.owner.getHeight then
			return max(self.owner:getWidth(), self.owner:getHeight()) * 0.5
		end
		return 0
	end,

	setAnchorPosition = function(self, pos)
		self.anchor.x = pos.x
		self.anchor.y = pos.y
	end,

	setFacingRight = function(self, facing_right)
		if self.facing_right == facing_right then
			return
		end
		self.facing_right = facing_right
		self:notify("onFlightFacingChanged", facing_right)
	end,

	setMode = function(self, mode)
		local state_name = self.state_names[mode] or mode
		if self.owner and self.owner.setState then
			self.owner:setState(state_name)
		end
		self:notify("onFlightModeChanged", mode)
	end,

	isMode = function(self, mode)
		local state_name = self.state_names[mode] or mode
		if not self.owner or not self.owner.isState then
			return false
		end
		return self.owner:isState(state_name)
	end,

	getStateElapsed = function(self)
		if not self.owner or not self.owner.getStateElapsed then
			return 0
		end
		return self.owner:getStateElapsed()
	end,

	restartState = function(self)
		if self.owner and self.owner.restartState then
			self.owner:restartState()
		end
	end,

	chooseExploreTarget = function(self)
		local world = self:getWorld()
		local owner = self.owner
		if not world or not owner or not self.explore_rule then
			return false
		end

		local target = world:resolveRule(self.explore_rule, false, self:getOwnerRadius())
		if not target then
			return false
		end

		self.explore_target.x = target.x
		self.explore_target.y = target.y
		self.explore_timer:reset()
		self.explore_retarget_time = random_int(self.explore_retarget_min, self.explore_retarget_max)
		return true
	end,

	getLandingRule = function(self)
		if not self.landing_rule then
			return nil
		end

		return {
			kind = self.landing_rule.kind,
			scope = self.landing_rule.scope,
			margin = self.landing_rule.margin,
			attempts = self.landing_rule.attempts,
			padding = self.landing_rule.padding,
			offset = self.landing_rule.offset,
			bounds = {
				left = self.anchor.x - self.landing_radius_x,
				top = self.anchor.y - self.landing_radius_y,
				right = self.anchor.x + self.landing_radius_x,
				bottom = self.anchor.y + self.landing_radius_y,
			},
		}
	end,

	beginFlying = function(self)
		if not self.owner then
			return
		end
		self.landing_target = nil
		self.owner.vel.x = 0
		self.owner.vel.y = 0
		self:setAnchorPosition(self.owner.pos)
		self.flutter_time = random_float(0, 6.28318)
		self.flight_duration = random_int(self.flight_duration_min, self.flight_duration_max)
		self:setMode("flying")
		self:chooseExploreTarget()
	end,

	tryBeginLanding = function(self)
		local owner = self.owner
		local world = self:getWorld()
		if not owner or not world or not self.landing_rule then
			return false
		end
		if owner.isAction and owner:isAction(Ecology.Actions.Flee) then
			return false
		end

		local landing_rule = self:getLandingRule()
		self.landing_target = world:resolveRule(landing_rule, false, self:getOwnerRadius())
		if not self.landing_target then
			return false
		end

		self:setMode("landing")
		return true
	end,

	fleeFromTarget = function(self, target)
		local owner = self.owner
		if not owner or not owner.getAwayVector then
			return false
		end

		local away = owner:getAwayVector(target)
		if not away then
			return false
		end

		away.y = min(away.y, self.flee_upward_clamp)
		local away_dist = sqrt(away.x * away.x + away.y * away.y)
		if away_dist > 0 then
			away.x = away.x / away_dist
			away.y = away.y / away_dist
		end

		self.landing_target = nil
		self:setMode("flying")
		owner.pos.x += away.x * self.flee_speed * _dt
		owner.pos.y += away.y * self.flee_speed * _dt
		self.anchor.x = owner.pos.x + away.x * self.flee_anchor_distance
		self.anchor.y = owner.pos.y + away.y * self.flee_anchor_distance
		self.explore_target.x = self.anchor.x
		self.explore_target.y = self.anchor.y
		self.explore_timer:reset()
		self:setFacingRight(away.x > 0)

		return true
	end,

	beginLanded = function(self)
		if not self.owner or not self.landing_target then
			return
		end

		self.owner.pos.x = self.landing_target.x
		self.owner.pos.y = self.landing_target.y
		self.owner.vel.x = 0
		self.owner.vel.y = 0
		self:setAnchorPosition(self.owner.pos)
		self.landed_duration = random_int(self.landed_duration_min, self.landed_duration_max)
		self:setMode("landed")
	end,

	afterSpawn = function(self)
		if not self.owner then
			return
		end
		self:setAnchorPosition(self.owner.pos)
		self:beginFlying()
	end,

	updateFlying = function(self)
		local owner = self.owner
		if not owner then
			return
		end

		if owner.isAction and owner:isAction(Ecology.Actions.Flee) then
			local target = owner.actions and owner.actions.data and owner.actions.data.target or self:getPlayer()
			self:fleeFromTarget(target)
			return
		end

		if self:getStateElapsed() >= self.flight_duration then
			if self:tryBeginLanding() then
				return
			end
			self:restartState()
		end

		if self.explore_timer:elapsed() >= self.explore_retarget_time then
			self:chooseExploreTarget()
		end

		self.flutter_time += _dt * self.flutter_frequency

		local target_x = self.explore_target.x
		local target_y = self.explore_target.y
		local dx = target_x - owner.pos.x
		local dy = target_y - owner.pos.y
		local dist = sqrt(dx * dx + dy * dy)

		if dist <= self.explore_speed * _dt or dist <= 8 then
			self:setAnchorPosition(owner.pos)
			self:chooseExploreTarget()
			dx = self.explore_target.x - owner.pos.x
			dy = self.explore_target.y - owner.pos.y
			dist = sqrt(dx * dx + dy * dy)
		end

		local travel_x = 0
		local travel_y = 0
		if dist > 0 then
			travel_x = (dx / dist) * self.explore_speed
			travel_y = (dy / dist) * self.explore_speed
			self.anchor.x = target_x
			self.anchor.y = target_y
		end

		local bob_y = sin(self.flutter_time) * self.flutter_amplitude
		owner.pos.x += travel_x * _dt
		owner.pos.y += (travel_y + bob_y) * _dt
		self:setFacingRight(travel_x > 0)
	end,

	updateLanding = function(self)
		local owner = self.owner
		if not owner then
			return
		end

		if owner.isAction and owner:isAction(Ecology.Actions.Flee) then
			self:beginFlying()
			local target = owner.actions and owner.actions.data and owner.actions.data.target or self:getPlayer()
			self:fleeFromTarget(target)
			return
		end

		if not self.landing_target then
			self:beginFlying()
			return
		end

		local dx = self.landing_target.x - owner.pos.x
		local dy = self.landing_target.y - owner.pos.y
		local dist = sqrt(dx * dx + dy * dy)
		self:setFacingRight(dx > 0)

		if dist <= self.landing_speed * _dt or dist <= 2 then
			self:beginLanded()
			return
		end

		if dist > 0 then
			owner.pos.x += (dx / dist) * self.landing_speed * _dt
			owner.pos.y += (dy / dist) * self.landing_speed * _dt
		end
	end,

	updateLanded = function(self)
		local owner = self.owner
		if not owner then
			return
		end

		if owner.isAction and owner:isAction(Ecology.Actions.Flee) then
			self:beginFlying()
			local target = owner.actions and owner.actions.data and owner.actions.data.target or self:getPlayer()
			self:fleeFromTarget(target)
			return
		end

		if self.landing_target then
			owner.pos.x = self.landing_target.x
			owner.pos.y = self.landing_target.y
		end

		if self:getStateElapsed() >= self.landed_duration then
			self:beginFlying()
		end
	end,

	updateControlled = function(self, input)
		local owner = self.owner
		if not owner then
			return
		end

		self.landing_target = nil
		self:setMode("flying")

		local dx = 0
		local dy = 0
		if input.left then
			dx -= 1
			owner.direction = 2
		end
		if input.right then
			dx += 1
			owner.direction = 3
		end
		if input.up then
			dy -= 1
			owner.direction = 0
		end
		if input.down then
			dy += 1
			owner.direction = 1
		end

		if dx ~= 0 or dy ~= 0 then
			local dist = sqrt(dx * dx + dy * dy)
			dx /= dist
			dy /= dist
			owner.pos.x += dx * self.explore_speed * _dt
			owner.pos.y += dy * self.explore_speed * _dt
			self:setFacingRight(dx > 0)
			self:setAnchorPosition(owner.pos)
			self.explore_target.x = owner.pos.x
			self.explore_target.y = owner.pos.y
		end
	end,

	updateAutonomous = function(self)
		if self:isMode("landing") then
			self:updateLanding()
		elseif self:isMode("landed") then
			self:updateLanded()
		else
			self:updateFlying()
		end
	end,
})

return Flight
