local Graphic = include("src/primitives/graphic.lua")
local Timer = include("src/classes/timer.lua")

local Wood = Graphic:new({
	_type = "Wood",
	seed = 13579,
	time_offset = 0,
	h = 20,
	ignore_physics = false,
	init = function(self)
		Graphic.init(self)
		self.wood_timer = Timer:new()
	end,
	update = function(self)
		Graphic.update(self)
	end,
	draw = function(self)
		local x = self.pos.x - self.w / 2
		local y = self.pos.y - self.h / 2

		self.layer.gfx.ProceduralTextures:wood(x, y, self.w, self.h, self.seed, 1, true)

		Graphic.draw(self)
	end
})

return Wood
