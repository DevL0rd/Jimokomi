local Class = include("src/Engine/Core/Class.lua")

local Controller = Class:new({
	_type = "Controller",
	owner = nil,
	directions = nil,

	getPlayerInputState = function(self)
		return {
			left = btn(0),
			right = btn(1),
			up = btn(2),
			down = btn(3),
			jump = btn(4),
		}
	end,

	getInputState = function(self)
		if self.owner and self.owner:isAutonomous() and self.owner.autonomy then
			return self.owner.autonomy:getInputState()
		end

		return self:getPlayerInputState()
	end,

	updateDirection = function(self, input)
		if input.up then
			self.owner.direction = self.directions.up
		elseif input.left then
			self.owner.direction = self.directions.left
		elseif input.right then
			self.owner.direction = self.directions.right
		elseif input.down then
			self.owner.direction = self.directions.down
		end
	end,
})

return Controller
