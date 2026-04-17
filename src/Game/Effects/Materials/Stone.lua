local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")
local Wave = include("src/Engine/Math/Wave.lua")

local function stone_noise(seed, x, y)
	local value = abs(sin((x + seed * 9) * 18.73 + (y + seed * 5) * 42.11))
	return value - flr(value)
end

local Stone = ProceduralSprite:new({
	_type = "Stone",
	seed = 67890,
	sprite = 0,
	length = 1,
	fps = 0,
	loop = true,
	cache_procedural_sprite = true,
	procedural_sprite_cache_mode = "surface",
	procedural_sprite_cache_key = "stone",
	debug = false,
	inherit_layer_debug = false,

	init = function(self)
		ProceduralSprite.init(self)
	end,

	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		target:drawVerticalGradient(0, 0, w, h, 5, 13)

		for y = 0, h - 1, 2 do
			local band = Wave.layered(y, 0, {
				{ frequency = 0.16, speed = 0, offset = self.seed * 0.01, weight = 1.0 },
				{ frequency = 0.47, speed = 0, offset = self.seed * 0.03, weight = 0.35 },
			})
			local color = band > 0.62 and 6 or (band < 0.32 and 1 or 13)
			target:line(0, y, w - 1, y, color)
		end

		for x = 1, w - 2, 4 do
			local crack_top = 1 + flr(Wave.triangle01(x * 0.21 + self.seed * 0.01) * (h * 0.35))
			local crack_len = 3 + flr(Wave.sine01(x * 0.13 + self.seed * 0.02) * 4)
			target:line(x, crack_top, x + 1, min(h - 1, crack_top + crack_len), 1)
		end

		for y = 1, h - 2 do
			for x = 1, w - 2, 2 do
				local noise = stone_noise(self.seed, x, y)
				if noise < 0.08 then
					target:pset(x, y, 7)
				elseif noise > 0.9 then
					target:pset(x, y, 1)
				end
			end
		end
	end
})

return Stone
