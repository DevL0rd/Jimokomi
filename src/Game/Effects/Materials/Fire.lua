local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")
local Wave = include("src/Engine/Math/Wave.lua")

local Fire = ProceduralSprite:new({
	_type = "Fire",
	seed = 54321,
	sprite = 0,
	length = 6,
	fps = 6,
	loop = true,
	cache_procedural_sprite = true,
	debug = false,
	inherit_layer_debug = false,

	init = function(self)
		ProceduralSprite.init(self)
	end,

	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		local phase = Wave.cycle01(frame_index or 0, self:getProceduralSpriteLength())
		target:drawVerticalGradient(0, 0, w, h, 8, 10)

		for x = 0, w - 1, 2 do
			local flame = Wave.layered(x, phase, {
				{ frequency = 0.18, speed = 1.4, offset = self.seed * 0.01, weight = 1.0 },
				{ frequency = 0.41, speed = -0.9, offset = self.seed * 0.03, weight = 0.55 },
			})
			local tip_y = max(0, flr((1 - flame) * (h * 0.7)))
			local core_y = min(h - 1, tip_y + 4 + flr(Wave.triangle01(x * 0.2 + phase * 2.0) * 3))
			target:rectfill(x, core_y, min(w - 1, x + 1), h - 1, 8)
			target:line(x, tip_y, min(w - 1, x + 1), core_y, 10)
			if core_y + 1 < h then
				target:line(x, core_y - 1, min(w - 1, x + 1), core_y - 1, 9)
			end
			if (x + flr(phase * 24)) % 5 == 0 and tip_y > 1 then
				target:pset(x, tip_y - 1, 7)
			end
		end

		for y = 2 + (flr(phase * 12) % 3), h - 4, 5 do
			for x = 1 + ((y * 2) % 5), w - 3, 7 do
				local ember_y = max(0, y - flr(Wave.triangle01(x * 0.11 + phase * 2.4) * 3))
				target:pset(x, ember_y, 10)
				if (x + y) % 2 == 0 then
					target:pset(x, min(h - 1, ember_y + 1), 9)
				end
			end
		end
	end
})

return Fire
