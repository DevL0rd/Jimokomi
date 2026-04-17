local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")

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
	init = function(self)
		ProceduralSprite.init(self)
	end,
	update = function(self)
		ProceduralSprite.update(self)
	end,
	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		target:drawVerticalGradient(0, 0, w, h, 5, 13)
		target:drawDitheredGradient(0, 0, w, h, 5, 6, 0xA55A)
		target:drawSpeckles(0, 0, w, h, { 6, 7, 13 }, self.seed, 0.06)
	end
})

return Stone
