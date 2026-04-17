local Material = include("src/Game/Effects/Materials/Material.lua")
local Timer = include("src/Engine/Core/Timer.lua")

local Fire = Material:new({
	_type = "Fire",
	seed = 54321,
	time_offset = 0,
	init = function(self)
		Material.init(self)
		self.fire_timer = Timer:new()
	end,
	update = function(self)
		self.time_offset = self.fire_timer:elapsed() * 0.001
		Material.update(self)
	end,
	drawMaterial = function(self, gfx, x, y, w, h)
		gfx:drawAnimatedGradient(x, y, w, h, { 8, 9, 10, 11, 7 }, 0.4)
		gfx:drawWaveGradient(x, y, w, h, 8, 10, 0.18, 0.45, self.time_offset * 3)
		gfx:drawDitheredGradient(x, y, w, h, 10, 9, 0x5A5A)
	end
})

return Fire
