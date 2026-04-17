local Class = include("src/Engine/Core/Class.lua")
local DebugOverlayLines = include("src/Engine/Core/DebugOverlay/Lines.lua")
local DebugOverlayObjects = include("src/Engine/Core/DebugOverlay/Objects.lua")
local DebugOverlaySummary = include("src/Engine/Core/DebugOverlay/Summary.lua")

local DebugOverlay = Class:new({
	_type = "DebugOverlay",
	max_entities = 5,

	appendLine = DebugOverlayLines.appendLine,
	drawLines = DebugOverlayLines.drawLines,
	getObjectPathing = DebugOverlayObjects.getObjectPathing,
	getObjectLine = DebugOverlayObjects.getObjectLine,
	drawMarker = DebugOverlayObjects.drawMarker,
	drawPath = DebugOverlayObjects.drawPath,
	drawCircleApprox = DebugOverlayObjects.drawCircleApprox,
	drawVisionCone = DebugOverlayObjects.drawVisionCone,
	drawObjectGuides = DebugOverlayObjects.drawObjectGuides,
	collectLayerLines = DebugOverlayObjects.collectLayerLines,
	drawLayerGuides = DebugOverlayObjects.drawLayerGuides,
	draw = DebugOverlaySummary.draw,
})

return DebugOverlay
