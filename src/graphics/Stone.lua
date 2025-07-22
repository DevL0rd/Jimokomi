local Graphic = include("src/primitives/graphic.lua")
local Timer = include("src/classes/timer.lua")

local Stone = Graphic:new({
	_type = "Stone",
	seed = 67890,
	time_offset = 0,
	init = function(self)
		Graphic.init(self)
		self.stone_timer = Timer:new()
	end,
	update = function(self)
		Graphic.update(self)
	end,
	draw = function(self)
		local x = self.pos.x - self.w / 2
		local y = self.pos.y - self.h / 2

		self.world.gfx.ProceduralTextures:stone(x, y, self.w, self.h, self.seed, 1, true)

		Graphic.draw(self)
	end
})

return Stone
