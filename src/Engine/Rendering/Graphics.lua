local Class = include("src/Engine/Core/Class.lua")
local GraphicsPrimitives = include("src/Engine/Rendering/Graphics/Primitives.lua")
local GraphicsRendering = include("src/Engine/Rendering/Graphics/Rendering.lua")

local Graphics = Class:new({
	_type = "Graphics",
	camera = nil,

	line = GraphicsPrimitives.line,
	screenLine = GraphicsPrimitives.screenLine,
	circ = GraphicsPrimitives.circ,
	screenCirc = GraphicsPrimitives.screenCirc,
	circfill = GraphicsPrimitives.circfill,
	screenCircfill = GraphicsPrimitives.screenCircfill,
	rect = GraphicsPrimitives.rect,
	screenRect = GraphicsPrimitives.screenRect,
	rectfill = GraphicsPrimitives.rectfill,
	screenRectfill = GraphicsPrimitives.screenRectfill,
	spr = GraphicsPrimitives.spr,
	print = GraphicsPrimitives.print,
	screenPrint = GraphicsPrimitives.screenPrint,
	drawHorizontalGradient = GraphicsRendering.drawHorizontalGradient,
	drawVerticalGradient = GraphicsRendering.drawVerticalGradient,
	drawDitheredGradient = GraphicsRendering.drawDitheredGradient,
	drawAnimatedGradient = GraphicsRendering.drawAnimatedGradient,
	drawWaveGradient = GraphicsRendering.drawWaveGradient,
	drawSpeckles = GraphicsRendering.drawSpeckles,
	drawSparkline = GraphicsRendering.drawSparkline,
	drawBarChart = GraphicsRendering.drawBarChart,
	drawPerfPanel = GraphicsRendering.drawPerfPanel,
})

return Graphics
