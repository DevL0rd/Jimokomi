local Graphic = include("src/primitives/graphic.lua")
local Timer = include("src/classes/timer.lua")

local Skybox = Graphic:new({
	_type = "Skybox",
	cycle_duration = 60000, -- 60 seconds for full day/night cycle (in milliseconds)
	world = nil,
	init = function(self)
		Graphic.init(self)
		if self.world then
			self.w = self.world.w
			self.h = self.world.h
			self.pos.x = self.world.w / 2
			self.pos.y = self.world.h / 2
		end
		self.cycle_timer = Timer:new()
	end,
	update = function(self)
		-- Reset timer when cycle completes
		if self.cycle_timer:hasElapsed(self.cycle_duration) then
			self.cycle_timer:reset()
		end
		Graphic.update(self)
	end,
	get_time_of_day = function(self)
		-- Returns a value from 0 to 1 representing the time of day
		-- 0 = midnight, 0.25 = dawn, 0.5 = noon, 0.75 = dusk, 1 = midnight again
		return self.cycle_timer:elapsed() / self.cycle_duration
	end,
	get_sky_colors = function(self)
		local time = self:get_time_of_day()

		-- Define color phases for different times of day
		local colors = {
			-- Dawn colors (0.2 - 0.3)
			dawn_top = 1, -- Dark blue
			dawn_bottom = 9, -- Orange

			-- Day colors (0.4 - 0.6)
			day_top = 12, -- Light blue
			day_bottom = 7, -- White/light cyan

			-- Dusk colors (0.7 - 0.8)
			dusk_top = 2, -- Purple
			dusk_bottom = 8, -- Red

			-- Night colors (0.9 - 0.1)
			night_top = 0, -- Black
			night_bottom = 1 -- Dark blue
		}

		local top_color, bottom_color

		if time >= 0 and time < 0.2 then
			-- Night to dawn transition
			local t = time / 0.2
			top_color = colors.night_top
			bottom_color = colors.night_bottom
		elseif time >= 0.2 and time < 0.3 then
			-- Dawn
			top_color = colors.dawn_top
			bottom_color = colors.dawn_bottom
		elseif time >= 0.3 and time < 0.4 then
			-- Dawn to day transition
			top_color = colors.day_top
			bottom_color = colors.day_bottom
		elseif time >= 0.4 and time < 0.6 then
			-- Day
			top_color = colors.day_top
			bottom_color = colors.day_bottom
		elseif time >= 0.6 and time < 0.7 then
			-- Day to dusk transition
			top_color = colors.day_top
			bottom_color = colors.day_bottom
		elseif time >= 0.7 and time < 0.8 then
			-- Dusk
			top_color = colors.dusk_top
			bottom_color = colors.dusk_bottom
		elseif time >= 0.8 and time < 1.0 then
			-- Dusk to night transition
			local t = (time - 0.8) / 0.2
			top_color = colors.night_top
			bottom_color = colors.night_bottom
		end

		return top_color, bottom_color
	end,
	draw = function(self)
		local top_color, bottom_color = self:get_sky_colors()

		-- Draw gradient from top to bottom
		for y = 0, self.world.h - 1 do
			local gradient_factor = y / self.world.h
			local current_color = top_color

			-- Simple color interpolation - you might want to make this more sophisticated
			if gradient_factor > 0.5 then
				current_color = bottom_color
			end

			-- Draw a horizontal line for this row
			self.world.gfx:line(0, y, self.world.w - 1, y, current_color)
		end

		Graphic.draw(self)
	end
})

return Skybox
