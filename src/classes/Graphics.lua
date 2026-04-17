local Class = include("src/classes/Class.lua")
local GraphicsPrimitives = include("src/classes/graphics/GraphicsPrimitives.lua")
local GraphicsProcedural = include("src/classes/graphics/GraphicsProcedural.lua")
local GraphicsGradients = include("src/classes/graphics/GraphicsGradients.lua")

local Graphics = Class:new({
	_type = "Graphics",
	camera = nil,
	procedural_textures = nil,

	init = function(self)
		GraphicsProcedural.initProceduralRenderer(self)
	end,

	line = GraphicsPrimitives.line,
	circ = GraphicsPrimitives.circ,
	circfill = GraphicsPrimitives.circfill,
	rect = GraphicsPrimitives.rect,
	rectfill = GraphicsPrimitives.rectfill,
	spr = GraphicsPrimitives.spr,
	print = GraphicsPrimitives.print,
	drawProceduralTexture = GraphicsProcedural.drawProceduralTexture,
	fire = GraphicsProcedural.fire,
	water = GraphicsProcedural.water,
	wood = GraphicsProcedural.wood,
	stone = GraphicsProcedural.stone,
	drawLinearGradient = GraphicsGradients.drawLinearGradient,
	drawRadialGradient = GraphicsGradients.drawRadialGradient,
	drawHorizontalGradient = GraphicsGradients.drawHorizontalGradient,
	drawVerticalGradient = GraphicsGradients.drawVerticalGradient,
	drawDitheredGradient = GraphicsGradients.drawDitheredGradient,
	drawPaletteGradient = GraphicsGradients.drawPaletteGradient,
	drawAnimatedGradient = GraphicsGradients.drawAnimatedGradient,
	drawWaveGradient = GraphicsGradients.drawWaveGradient,
	setDitherPattern = GraphicsGradients.setDitherPattern,
	resetFillPattern = GraphicsGradients.resetFillPattern,
})

return Graphics
