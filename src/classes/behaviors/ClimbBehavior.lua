local Behavior = include("src/classes/behaviors/Behavior.lua")

local ClimbBehavior = Behavior:new({
	_type = "ClimbBehavior",
	speed = 120,

	updateMovement = function(self, input)
		if not self.owner then
			return
		end

		input = input or {}
		self.owner.ignore_gravity = true
		self.owner.vel.x = 0
		self.owner.vel.y = 0

		if input.left then
			self.owner.vel.x = -self.speed
		elseif input.right then
			self.owner.vel.x = self.speed
		end

		if input.up then
			self.owner.vel.y = -self.speed
		elseif input.down then
			self.owner.vel.y = self.speed
		end
	end,

	stop = function(self)
		if not self.owner then
			return
		end
		self.owner.ignore_gravity = false
	end,
})

return ClimbBehavior
