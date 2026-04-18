local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")
local Wave = include("src/Engine/Math/Wave.lua")

local Wood = ProceduralSprite:new({
	_type = "Wood",
	seed = 13579,
	sprite = 0,
	length = 1,
	fps = 0,
	loop = true,
	cache_procedural_sprite = true,
	debug = false,
	inherit_layer_debug = false,
	shape = {
		kind = "rect",
		w = 16,
		h = 20,
	},
	ignore_physics = false,

	init = function(self)
		ProceduralSprite.init(self)
	end,

	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		target:drawHorizontalGradient(0, 0, w, h, 4, 9)

		for x = 0, w - 1 do
			local grain = Wave.layered(x, 0, {
				{ frequency = 0.22, speed = 0, offset = self.seed * 0.01, weight = 1.0 },
				{ frequency = 0.51, speed = 0, offset = self.seed * 0.03, weight = 0.45 },
			})
			local tone = grain > 0.62 and 1 or (grain < 0.34 and 15 or 4)
			target:line(x, 0, x, h - 1, tone)
			if x % 4 == 1 then
				target:line(x, 0, x, h - 1, 9)
			end
		end

		local knot_y = 4 + (self.seed % max(1, h - 8))
		local knot_x = 3 + (self.seed % max(1, w - 6))
		target:circ(knot_x, knot_y, 2, 1)
		target:circfill(knot_x, knot_y, 1, 4)

		for y = 2, h - 3, 5 do
			local seam_shift = flr(Wave.triangle01(y * 0.18 + self.seed * 0.01) * 3)
			target:line(1 + seam_shift, y, min(w - 2, w - 3 + seam_shift), y, 15)
		end
	end
})

return Wood
