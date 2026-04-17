local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")

local Wood = ProceduralSprite:new({
	_type = "Wood",
	seed = 13579,
	sprite = 0,
	length = 1,
	fps = 0,
	loop = true,
	cache_procedural_sprite = true,
	procedural_sprite_cache_mode = "surface",
	procedural_sprite_cache_key = "wood",
	shape = {
		kind = "rect",
		w = 16,
		h = 20,
	},
	ignore_physics = false,
	init = function(self)
		ProceduralSprite.init(self)
	end,
	update = function(self)
		ProceduralSprite.update(self)
	end,
	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		target:drawHorizontalGradient(0, 0, w, h, 4, 9)
		target:drawDitheredGradient(0, 0, w, h, 4, 15, 0x6666)
		for i = 2, w - 1, 4 do
			target:line(i, 0, i, h - 1, 1)
		end
	end
})

return Wood
