local Class = include("src/Engine/Core/Class.lua")
local GraphicsPrimitives = include("src/Engine/Rendering/Graphics/Primitives.lua")
local GraphicsRendering = include("src/Engine/Rendering/Graphics/Rendering.lua")
local GraphicsPanels = include("src/Engine/Rendering/Graphics/Panels.lua")
local GraphicsCache = include("src/Engine/Rendering/Graphics/Cache.lua")
local GraphicsRetained = include("src/Engine/Rendering/Graphics/Retained.lua")

local Graphics = Class:new({
	_type = "Graphics",
	camera = nil,
	profiler = nil,
	panel_cache = nil,
	render_cache = nil,
	cache_renders = true,

	init = function(self)
		self.panel_cache = {}
		self.render_cache = {}
	end,

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
	appendPerfPanelCommands = GraphicsRendering.appendPerfPanelCommands,
	drawPerfPanel = GraphicsRendering.drawPerfPanel,
	clearPanelCache = GraphicsPanels.clearPanelCache,
	getCachedPanel = GraphicsPanels.getCachedPanel,
	appendPanelCommands = GraphicsPanels.appendPanelCommands,
	drawPanel = GraphicsPanels.drawPanel,
	newRetainedPanel = GraphicsPanels.newRetainedPanel,
	clearRenderCache = GraphicsCache.clearRenderCache,
	setRenderCacheEnabled = GraphicsCache.setRenderCacheEnabled,
	getCachedRender = GraphicsCache.getCachedRender,
	ensureCachedRender = GraphicsCache.ensureCachedRender,
	ensureCachedSurface = GraphicsCache.ensureCachedSurface,
	buildImmediateTarget = GraphicsCache.buildImmediateTarget,
	recordCommandList = GraphicsCache.recordCommandList,
	replayCommandList = GraphicsCache.replayCommandList,
	appendOffsetCommands = GraphicsCache.appendOffsetCommands,
	drawCachedScreen = GraphicsCache.drawCachedScreen,
	drawCachedSurfaceScreen = GraphicsCache.drawCachedSurfaceScreen,
	drawCachedLayer = GraphicsCache.drawCachedLayer,
	drawCachedSurfaceLayer = GraphicsCache.drawCachedSurfaceLayer,
	newRetainedLeaf = GraphicsRetained.newRetainedLeaf,
	newRetainedGroup = GraphicsRetained.newRetainedGroup,
	attachRetainedGfx = GraphicsRetained.attachRetainedGfx,
})

return Graphics
