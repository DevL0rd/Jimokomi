local Material = include("src/Game/Effects/Materials/Material.lua")
local Timer = include("src/Engine/Core/Timer.lua")

local Water = Material:new({
	_type = "Water",
	seed = 12345,
	init = function(self)
		Material.init(self)
		self.animtion_timer = Timer:new()
	end,
	update = function(self)
		Material.update(self)
	end,
	drawMaterial = function(self, gfx, x, y, w, h)
		local time_elapsed_seconds = round(self.animtion_timer:elapsed() / 1000)
		if time_elapsed_seconds > 5 then
			self.animtion_timer:reset()
		end
		gfx:drawVerticalGradient(x, y, w, h, 12, 1)
		gfx:drawWaveGradient(x, y, w, h, 12, 6, 0.15, 0.4, time_elapsed_seconds)
		gfx:drawDitheredGradient(x, y, w, h, 12, 7, 0x3C3C)
	end
})

return Water
