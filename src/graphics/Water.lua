local Graphic = include("src/primitives/graphic.lua")
local Timer = include("src/classes/timer.lua")

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
		local x = self.pos.x - self.w / 2
		local y = self.pos.y - self.h / 2
		local time_elapsed_seconds = round(self.animtion_timer:elapsed() / 1000) -- Convert to seconds
		if time_elapsed_seconds > 5 then
			self.animtion_timer:reset()                                    -- Reset timer if it exceeds 5 seconds
		end
		self.world.gfx.ProceduralTextures:water(x, y, self.w, self.h, self.seed, time_elapsed_seconds,
			true)
		Graphic.draw(self)
	end
})

return Water
