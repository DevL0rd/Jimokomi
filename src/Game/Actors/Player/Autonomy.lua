local Class = include("src/Engine/Core/Class.lua")
local Timer = include("src/Engine/Core/Timer.lua")
local Ecology = include("src/Game/Ecology/Ecology.lua")
local Pathing = include("src/Game/Actors/Locomotion/Pathing.lua")
local WalkClimbJumpPath = include("src/Game/Actors/Locomotion/Pathing/WalkClimbJump.lua")

local Autonomy = Class:new({
	_type = "Autonomy",
	owner = nil,
	wander_rule = nil,
	wander_target = nil,
	path = nil,
	path_index = 1,
	path_target = nil,
	wander_retarget_min = 1500,
	wander_retarget_max = 4000,
	prey_seek_radius = 120,
	direct_seek_distance = 28,
	horizontal_deadzone = 6,
	vertical_deadzone = 10,
	jump_target_height = 18,
	blocked_probe_padding = 3,

	init = function(self)
		self.wander_retarget_min = self.wander_retarget_min or 1500
		self.wander_retarget_max = self.wander_retarget_max or self.wander_retarget_min
		if self.wander_retarget_max < self.wander_retarget_min then
			self.wander_retarget_max = self.wander_retarget_min
		end
		self.horizontal_deadzone = self.horizontal_deadzone or 6
		self.vertical_deadzone = self.vertical_deadzone or 10
		self.jump_target_height = self.jump_target_height or 18
		self.blocked_probe_padding = self.blocked_probe_padding or 3
		self.path_retarget_distance = self.path_retarget_distance or 20
		self.wander_timer = Timer:new()
		self.wander_target = self.wander_target or { x = 0, y = 0 }
		self.direct_seek_distance = self.direct_seek_distance or 28
		self.pathing = Pathing:new({
			owner = self.owner,
			policy = WalkClimbJumpPath,
			repath_distance = self.path_retarget_distance,
			repath_rate_hz = 1,
			repath_fail_rate_hz = 0.5,
		})
		self.path = nil
		self.path_index = 1
		self.path_target = nil
		self.next_wander_retarget = random_int(self.wander_retarget_min, self.wander_retarget_max)
		self.wander_rule = self.wander_rule or {
			kind = "map_random",
			scope = "map",
			margin = 32,
			padding = 32,
			attempts = 24,
		}
	end,

	resetInput = function(self)
		return {
			left = false,
			right = false,
			up = false,
			down = false,
			jump = false,
		}
	end,

	getCurrentAction = function(self)
		return self.owner and self.owner.getCurrentAction and self.owner:getCurrentAction() or nil
	end,

	getCurrentTarget = function(self)
		if not self.owner or not self.owner.actions or not self.owner.actions.data then
			return nil
		end
		return self.owner.actions.data.target
	end,

	isClimbable = function(self)
		if not self.owner or not self.owner.locomotion then
			return false
		end
		local tile_id = self.owner.locomotion:getCurrentTile()
		return self.owner.locomotion:isClimbableTile(tile_id)
	end,

	isOnGround = function(self)
		return self.owner and self.owner.locomotion and self.owner.locomotion:isOnGround() or false
	end,

	isBlockedAhead = function(self, direction)
		local owner = self.owner
		local world = owner and owner:getWorld()
		if not owner or not world or not owner.getRadius then
			return false
		end
		local radius = owner:getRadius()
		local probe_x = owner.pos.x + direction * (radius + self.blocked_probe_padding)
		local probe_y = owner.pos.y
		return world:isSolidAt(probe_x, probe_y)
	end,

	chooseWanderTarget = function(self)
		local owner = self.owner
		if not owner or not self.pathing or not self.pathing.policy or not self.pathing.policy.getRandomReachableTarget then
			return false
		end
		local target = self.pathing.policy:getRandomReachableTarget(self.pathing, self.wander_rule)
		if not target then
			return false
		end
		self.wander_target.x = target.x
		self.wander_target.y = target.y
		self.wander_timer:reset()
		self.next_wander_retarget = random_int(self.wander_retarget_min, self.wander_retarget_max)
		return true
	end,

	invalidatePath = function(self)
		if self.pathing then
			self.pathing:invalidate()
		end
		self.path = nil
		self.path_index = 1
		self.path_target = nil
	end,

	setPathTarget = function(self, target)
		self.path_target = target and { x = target.x, y = target.y } or nil
		self.path_index = 1
	end,

	needsNewPath = function(self, target)
		return self.pathing and self.pathing:needsRepath(target) or true
	end,

	rebuildPath = function(self, target)
		if not self.pathing or not target then
			self:invalidatePath()
			return nil
		end
		self.path = self.pathing:rebuild(target)
		self.path_index = self.pathing.path_index or 1
		self.path_target = self.pathing.path_goal
		return self.path
	end,

	getPathWaypoint = function(self, target)
		if not self.owner or not target or not self.pathing then
			return target
		end
		local dx = target.x - self.owner.pos.x
		local dy = target.y - self.owner.pos.y
		if dx * dx + dy * dy <= self.direct_seek_distance * self.direct_seek_distance then
			self.pathing:invalidate()
			self.path = nil
			self.path_index = 1
			self.path_target = nil
			return target
		end
		local waypoint = self.pathing:getWaypoint(target, 12)
		self.path = self.pathing.path
		self.path_index = self.pathing.path_index
		self.path_target = self.pathing.path_goal
		return waypoint or target
	end,

	ensureWanderTarget = function(self)
		local owner = self.owner
		if not owner then
			return nil
		end

		local target = self.wander_target or { x = owner.pos.x, y = owner.pos.y }
		self.wander_target = target
		local dx = target.x - owner.pos.x
		local dy = target.y - owner.pos.y
		local dist2 = dx * dx + dy * dy
		if dist2 <= 14 * 14 or self.wander_timer:elapsed() >= self.next_wander_retarget then
			self:chooseWanderTarget()
		end
		return self.wander_target
	end,

	applyTargetInput = function(self, input, target)
		local owner = self.owner
		if not owner or not target then
			return input
		end

		local dx = target.x - owner.pos.x
		local dy = target.y - owner.pos.y
		local climbable = self:isClimbable()

		if dx < -self.horizontal_deadzone then
			input.left = true
		elseif dx > self.horizontal_deadzone then
			input.right = true
		end

		if climbable and dy < -self.vertical_deadzone then
			input.up = true
		elseif climbable and dy > self.vertical_deadzone then
			input.down = true
		elseif not climbable and self:isOnGround() and dy < -self.jump_target_height then
			input.jump = true
		end

		if self:isOnGround() then
			if input.left and self:isBlockedAhead(-1) then
				input.jump = true
			elseif input.right and self:isBlockedAhead(1) then
				input.jump = true
			end
		end

		return input
	end,

	getInputState = function(self)
		local input = self:resetInput()
		local action = self:getCurrentAction()
		local target = self:getCurrentTarget()

		if action ~= Ecology.Actions.Hunt or not target or target._delete or not target.pos then
			target = self:ensureWanderTarget()
		else
			target = target.pos
		end

		target = self:getPathWaypoint(target)
		return self:applyTargetInput(input, target)
	end,
})

return Autonomy
