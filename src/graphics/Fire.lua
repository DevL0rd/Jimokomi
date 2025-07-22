local Graphic = include("src/classes/Graphic.lua")
local Timer = include("src/classes/timer.lua")

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
		local x = self.pos.x - self.w / 2
		local y = self.pos.y - self.h / 2

		self.world.gfx.ProceduralTextures:fire(x, y, self.w, self.h, self.seed, self.time_offset)

		Graphic.draw(self)
	end
})

return Fire
