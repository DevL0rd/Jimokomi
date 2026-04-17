local Graphic = include("src/primitives/Graphic.lua")
local Timer = include("src/classes/Timer.lua")

local Water = Graphic:new({
	_type = "Water",
	seed = 12345,
	init = function(self)
		Graphic.init(self)
		self.animtion_timer = Timer:new()
	end,
	update = function(self)
		Graphic.update(self)
	end,
	draw = function(self)
		local top_left = self:getTopLeft()
		local width = self:getWidth()
		local height = self:getHeight()
		local time_elapsed_seconds = round(self.animtion_timer:elapsed() / 1000) -- Convert to seconds
		if time_elapsed_seconds > 5 then
			self.animtion_timer:reset()                                    -- Reset timer if it exceeds 5 seconds
		end
		self.layer.gfx:water(top_left.x, top_left.y, width, height, self.seed, time_elapsed_seconds, true)
		Graphic.draw(self)
	end
})

return Water
