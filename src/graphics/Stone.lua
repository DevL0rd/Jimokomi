local Graphic = include("src/primitives/Graphic.lua")
local Timer = include("src/classes/Timer.lua")

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
		local top_left = self:getTopLeft()
		local width = self:getWidth()
		local height = self:getHeight()

		self.layer.gfx:stone(top_left.x, top_left.y, width, height, self.seed, 1, true)

		Graphic.draw(self)
	end
})

return Stone
