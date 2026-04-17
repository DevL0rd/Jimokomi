local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")

local Skybox = ProceduralSprite:new({
	_type = "Skybox",
	sprite = 0,
	length = 8,
	fps = 8 / 60,
	loop = true,
	cache_procedural_sprite = true,
	procedural_sprite_cache_mode = "surface",
	procedural_sprite_cache_key = "skybox",
	layer = nil,
	init = function(self)
		ProceduralSprite.init(self)
		if self.layer then
			self:setRectShape(self.layer.w, self.layer.h)
			self.pos.x = self.layer.w / 2
			self.pos.y = self.layer.h / 2
		end
	end,
	update = function(self)
		ProceduralSprite.update(self)
	end,
	get_time_of_day = function(self, frame_index)
		local frame_count = max(1, self:getProceduralSpriteLength())
		if frame_count <= 1 then
			return 0
		end
		return (frame_index or 0) / (frame_count - 1)
	end,
	get_sky_colors = function(self, frame_index)
		local time = self:get_time_of_day(frame_index)

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
	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		local top_color, bottom_color = self:get_sky_colors(frame_index)
		target:drawVerticalGradient(0, 0, w, h, top_color, bottom_color)
	end
})

return Skybox
