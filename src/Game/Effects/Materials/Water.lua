local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")
local Wave = include("src/Engine/Math/Wave.lua")

local Water = ProceduralSprite:new({
	_type = "Water",
	seed = 12345,
	sprite = 0,
	length = 8,
	fps = 6,
	loop = true,
	cache_procedural_sprite = true,
	debug = false,
	inherit_layer_debug = false,
	ignore_collisions = true,
	init = function(self)
		ProceduralSprite.init(self)
	end,
	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		local phase01 = Wave.cycle01(frame_index or 0, self:getProceduralSpriteLength())
		local phase = phase01
		local base_surface = mid(h * 0.18, flr(h * 0.24), h - 6)
		local surface_layers = {
			{ frequency = 0.06, speed = 0.90, offset = self.seed * 0.01, weight = 1.0 },
			{ frequency = 0.11, speed = -0.65, offset = self.seed * 0.02, weight = 0.55 },
			{ frequency = 0.23, speed = 1.30, offset = self.seed * 0.03, weight = 0.2 },
		}

		for x = 0, w - 1, 2 do
			local x2 = min(w - 1, x + 1)
			local wave_height = Wave.layered(x, phase, surface_layers)
			local crest_offset = Wave.quantize((wave_height - 0.5) * 6, 4)
			local surface_y = mid(0, flr(base_surface + crest_offset), h - 4)
			local foam_shift = Wave.triangle01(x * 0.085 + phase * 1.40 + self.seed * 0.005)
			local foam_length = 1 + flr(foam_shift * 2)

			target:rectfill(x, surface_y + 2, x2, h - 1, 1)
			target:line(x, min(h - 1, surface_y + 1), x2, min(h - 1, surface_y + 1), 12)
			target:line(x, surface_y, x2, surface_y, 6)

			if (x + flr(phase01 * 32)) % 7 < foam_length then
				target:pset(x, max(0, surface_y - 1), 7)
			end

			if surface_y + 4 < h and x % 4 == 0 then
				target:pset(x, surface_y + 4, 12)
			end
		end

		for y = base_surface + 6, h - 4, 7 do
			local band_shift = flr((phase01 * 18 + y) % 14)
			for x = band_shift, w - 4, 14 do
				local band_y = y + flr(Wave.triangle01(x * 0.08 + phase * 1.0 + y * 0.03) * 1.5)
				if band_y < h - 1 then
					target:line(x, band_y, min(w - 1, x + 3), band_y, 12)
				end
			end
		end

		for x = 4 + (flr(phase01 * 16) % 6), w - 4, 15 do
			local glint_y = base_surface + 2 + flr(Wave.sine01(x * 0.14 + phase * 1.6) * 3)
			if glint_y < h - 2 then
				target:line(x, glint_y, min(w - 1, x + 2), glint_y, 6)
			end
		end
	end
})

return Water
