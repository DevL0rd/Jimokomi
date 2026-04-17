local Material = include("src/Game/Effects/Materials/Material.lua")

local Wood = Material:new({
	_type = "Wood",
	seed = 13579,
	shape = {
		kind = "rect",
		w = 16,
		h = 20,
	},
	ignore_physics = false,
	init = function(self)
		Material.init(self)
	end,
	update = function(self)
		Material.update(self)
	end,
	drawMaterial = function(self, gfx, x, y, w, h)
		gfx:drawHorizontalGradient(x, y, w, h, 4, 9)
		gfx:drawDitheredGradient(x, y, w, h, 4, 15, 0x6666)
		for i = 2, w - 1, 4 do
			local sx, sy = gfx.camera:layerToScreenXY(x + i, y)
			line(sx, sy, sx, sy + h - 1, 1)
		end
	end
})

return Wood
