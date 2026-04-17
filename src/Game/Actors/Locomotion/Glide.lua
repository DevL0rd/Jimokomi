local Mode = include("src/Game/Actors/Locomotion/Mode.lua")

local Glide = Mode:new({
	_type = "Glide",
	horizontal_accel = 180,
	lift = 2,

	updateControlled = function(self, input)
		if not self.owner then
			return
		end

		input = input or {}
		self.owner.vel.y -= self.lift

		if input.left then
			self.owner.vel.x -= self.horizontal_accel * _dt
		elseif input.right then
			self.owner.vel.x += self.horizontal_accel * _dt
		end
	end,
})

return Glide
