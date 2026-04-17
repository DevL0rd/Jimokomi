local Class = include("src/classes/Class.lua")
local DebugOverlayLines = include("src/classes/debug/DebugOverlayLines.lua")
local DebugOverlayObjects = include("src/classes/debug/DebugOverlayObjects.lua")
local DebugOverlaySummary = include("src/classes/debug/DebugOverlaySummary.lua")

local DebugOverlay = Class:new({
	_type = "DebugOverlay",
	max_entities = 5,

	appendLine = DebugOverlayLines.appendLine,
	drawLines = DebugOverlayLines.drawLines,
	getObjectLine = DebugOverlayObjects.getObjectLine,
	collectLayerLines = DebugOverlayObjects.collectLayerLines,
	draw = DebugOverlaySummary.draw,
})

return DebugOverlay
