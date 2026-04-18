local Screen = include("src/Engine/Core/Screen.lua")
local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")
local Wave = include("src/Engine/Math/Wave.lua")

local Skybox = ProceduralSprite:new({
	_type = "Skybox",
	seed = 24680,
	sprite = 0,
	length = 1,
	fps = 0,
	loop = true,
	cache_procedural_sprite = true,
	draw_behind_map = true,
	debug = false,
	inherit_layer_debug = false,
	ignore_collisions = true,
	cycle_duration = 120,
	cache_refresh_hz = 4,

	init = function(self)
		ProceduralSprite.init(self)
		if self.layer then
			self:setRectShape(self.layer.w, self.layer.h)
			self.pos.x = self.layer.w * 0.5
			self.pos.y = self.layer.h * 0.5
		end
	end,

	getSkyPhase = function(self)
		local duration = max(1, self.cycle_duration or 90)
		return (time() % duration) / duration
	end,

	getSkyCacheFrame = function(self)
		local refresh_hz = max(1, self.cache_refresh_hz or 4)
		return flr(time() * refresh_hz)
	end,

	drawSky = function(self, target, w, h, phase)
		local horizon_wave = Wave.sine01(phase * 6.28318 + self.seed * 0.0001)
		local horizon_y = flr(h * (0.58 + (horizon_wave - 0.5) * 0.05))
		local haze_band_y = max(0, horizon_y - 12)
		local lower_band_h = max(1, h - horizon_y)

		target:drawVerticalGradient(0, 0, w, horizon_y, 1, 1)
		target:drawVerticalGradient(0, horizon_y, w, lower_band_h, 1, 2)

		for y = haze_band_y, min(h - 1, horizon_y + 8), 2 do
			target:line(0, y, w - 1, y, 2)
		end
	end,

	drawBackground = function(self, layer)
		if not layer or not layer.gfx then
			return
		end
		local gfx = layer.gfx
		local phase = self:getSkyPhase()
		local cache_frame = self:getSkyCacheFrame()
		gfx:drawCachedScreen(
			nil,
			0,
			0,
			function(target)
				self:drawSky(target, Screen.w, Screen.h, phase)
			end,
				{
					w = Screen.w,
					h = Screen.h,
					cache_tag = "background.skybox",
					cache_profile_key = "background.skybox",
					cache_profile_signature = table.concat({
						tostr(Screen.w),
						tostr(Screen.h),
						tostr(self.cache_refresh_hz or 4),
						tostr(cache_frame),
					}, ":"),
					clear_color = 0,
				}
			)
	end,

	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		self:drawSky(target, w, h, self:getSkyPhase())
	end,
})

return Skybox
