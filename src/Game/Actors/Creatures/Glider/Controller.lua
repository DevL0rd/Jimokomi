local Class = include("src/Engine/Core/Class.lua")

local Controller = Class:new({
	_type = "Controller",
	owner = nil,
	directions = nil,

	getPlayerInputState = function(self)
		self.player_input_state = self.player_input_state or {}
		local input = self.player_input_state
		input.left = btn(0)
		input.right = btn(1)
		input.up = btn(2)
		input.down = btn(3)
		input.jump = btn(4)
		return input
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
