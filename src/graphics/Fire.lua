local Graphic = include("src/primitives/Graphic.lua")
local Timer = include("src/classes/Timer.lua")

local Fire = Graphic:new({
	_type = "Fire",
	seed = 54321,
	time_offset = 0,
	init = function(self)
		Graphic.init(self)
		self.fire_timer = Timer:new()
	end,
	update = function(self)
		-- Update time offset for animated fire
		self.time_offset = self.fire_timer:elapsed() * 0.001 -- Convert to appropriate scale
		Graphic.update(self)
	end,
	draw = function(self)
		local top_left = self:getTopLeft()
		local width = self:getWidth()
		local height = self:getHeight()

		self.layer.gfx:fire(top_left.x, top_left.y, width, height, self.seed, self.time_offset)

		Graphic.draw(self)
	end
})

return Fire
