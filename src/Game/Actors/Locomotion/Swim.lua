local Mode = include("src/Game/Actors/Locomotion/Mode.lua")

local Swim = Mode:new({
	_type = "Swim",
	horizontal_accel = 90,
	vertical_accel = 70,
	buoyancy = 18,
	drag = 0.1,

	updateControlled = function(self, input)
		if not self.owner then
			return
		end

		input = input or {}
		self.owner.vel.y -= self.buoyancy * _dt

		if input.left then
			self.owner.vel.x -= self.horizontal_accel * _dt
		elseif input.right then
			self.owner.vel.x += self.horizontal_accel * _dt
		end

		if input.up then
			self.owner.vel.y -= self.vertical_accel * _dt
		elseif input.down then
			self.owner.vel.y += self.vertical_accel * _dt
		end

		self.owner.vel.x = self.owner.vel.x * (1 - self.drag)
		self.owner.vel.y = self.owner.vel.y * (1 - self.drag)
	end,
})

return Swim
