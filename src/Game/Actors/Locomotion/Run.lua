local Mode = include("src/Game/Actors/Locomotion/Mode.lua")

local Run = Mode:new({
	_type = "Run",
	horizontal_accel = 180,
	ground_push = 8,

	updateControlled = function(self, input)
		if not self.owner then
			return
		end
		input = input or {}
		if input.left then
			self.owner.vel.x -= self.horizontal_accel * _dt
		elseif input.right then
			self.owner.vel.x += self.horizontal_accel * _dt
		end
		if self.owner.isOnGround and self.owner:isOnGround() then
			self.owner.vel.y -= self.ground_push
		end
	end,
})

return Run
