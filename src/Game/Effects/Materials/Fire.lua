local ProceduralSprite = include("src/Engine/Objects/ProceduralSprite.lua")

local Fire = ProceduralSprite:new({
	_type = "Fire",
	seed = 54321,
	sprite = 0,
	length = 6,
	fps = 6,
	loop = true,
	cache_procedural_sprite = true,
	procedural_sprite_cache_mode = "surface",
	procedural_sprite_cache_key = "fire",
	init = function(self)
		ProceduralSprite.init(self)
	end,
	update = function(self)
		ProceduralSprite.update(self)
	end,
	drawProceduralSprite = function(self, target, w, h, frame_id, frame_index)
		local phase = frame_index or 0
		target:drawAnimatedGradient(0, 0, w, h, { 8, 9, 10, 11, 7 }, 0.4, phase)
		target:drawWaveGradient(0, 0, w, h, 8, 10, 0.18, 0.45, phase * 3)
		target:drawDitheredGradient(0, 0, w, h, 10, 9, 0x5A5A)
	end
})

return Fire
