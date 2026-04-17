local Class = include("src/Engine/Core/Class.lua")
local GraphicsPrimitives = include("src/Engine/Rendering/Graphics/Primitives.lua")
local GraphicsRendering = include("src/Engine/Rendering/Graphics/Rendering.lua")

local Graphics = Class:new({
	_type = "Graphics",
	camera = nil,

	line = GraphicsPrimitives.line,
	circ = GraphicsPrimitives.circ,
	circfill = GraphicsPrimitives.circfill,
	rect = GraphicsPrimitives.rect,
	rectfill = GraphicsPrimitives.rectfill,
	spr = GraphicsPrimitives.spr,
	print = GraphicsPrimitives.print,
	drawHorizontalGradient = GraphicsRendering.drawHorizontalGradient,
	drawVerticalGradient = GraphicsRendering.drawVerticalGradient,
	drawDitheredGradient = GraphicsRendering.drawDitheredGradient,
	drawAnimatedGradient = GraphicsRendering.drawAnimatedGradient,
	drawWaveGradient = GraphicsRendering.drawWaveGradient,
	drawSpeckles = GraphicsRendering.drawSpeckles,
})

return Graphics
