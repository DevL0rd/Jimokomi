local Graphic = include("src/primitives/Graphic.lua")
local Timer = include("src/classes/Timer.lua")

local Wood = Graphic:new({
	_type = "Wood",
	seed = 13579,
	time_offset = 0,
	shape = {
		kind = "rect",
		w = 16,
		h = 20,
	},
	ignore_physics = false,
	init = function(self)
		Graphic.init(self)
		self.wood_timer = Timer:new()
	end,
	update = function(self)
		Graphic.update(self)
	end,
	draw = function(self)
		local top_left = self:getTopLeft()
		local width = self:getWidth()
		local height = self:getHeight()

		self.layer.gfx:wood(top_left.x, top_left.y, width, height, self.seed, 1, true)

		Graphic.draw(self)
	end
})

return Wood
