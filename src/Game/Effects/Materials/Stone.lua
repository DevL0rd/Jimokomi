local Material = include("src/Game/Effects/Materials/Material.lua")

local Stone = Material:new({
	_type = "Stone",
	seed = 67890,
	init = function(self)
		Material.init(self)
	end,
	update = function(self)
		Material.update(self)
	end,
	drawMaterial = function(self, gfx, x, y, w, h)
		gfx:drawVerticalGradient(x, y, w, h, 5, 13)
		gfx:drawDitheredGradient(x, y, w, h, 5, 6, 0xA55A)
		gfx:drawSpeckles(x, y, w, h, { 6, 7, 13 }, self.seed, 0.06)
	end
})

return Stone
