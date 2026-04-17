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
	appendMarker = DebugOverlayObjects.appendMarker,
	appendPath = DebugOverlayObjects.appendPath,
	appendCircleApprox = DebugOverlayObjects.appendCircleApprox,
	appendVisionCone = DebugOverlayObjects.appendVisionCone,
	appendObjectGuides = DebugOverlayObjects.appendObjectGuides,
	collectLayerLines = DebugOverlayObjects.collectLayerLines,
	drawLayerGuides = DebugOverlayObjects.drawLayerGuides,
	draw = DebugOverlaySummary.draw,
})

return DebugOverlay
