local Vector = include("src/classes/Vector.lua")
local EngineServices = include("src/classes/engine/EngineServices.lua")
local EngineLayers = include("src/classes/engine/EngineLayers.lua")
local EngineLoop = include("src/classes/engine/EngineLoop.lua")
local EngineDebug = include("src/classes/engine/EngineDebug.lua")
local EngineSnapshot = include("src/classes/engine/EngineSnapshot.lua")

local Engine = {
	layers = {},
	master_camera = nil,
	clock = nil,
	debug_buffer = nil,
	assets = nil,
	debug_overlay = nil,
	next_object_id = 0,
	sorted_layer_ids = {},
	debug = DEBUG or false,
	w = 16 * 64,
	h = 16 * 32,
	gravity = Vector:new({
		y = 200
	}),
	fill_color = 1,
	stroke_color = 5,
	ensureServices = EngineServices.ensureServices,
	nextObjectId = EngineServices.nextObjectId,
	createLayer = EngineLayers.createLayer,
	getLayer = EngineLayers.getLayer,
	on = EngineServices.on,
	once = EngineServices.once,
	off = EngineServices.off,
	emit = EngineServices.emit,
	linkCameras = EngineLayers.linkCameras,
	setMasterCamera = EngineLayers.setMasterCamera,
	appendDebug = EngineServices.appendDebug,
	clearDebug = EngineServices.clearDebug,
	update = EngineLoop.update,
	draw = EngineLoop.draw,
	start = EngineLoop.start,
	stop = EngineLoop.stop,
	debug_state = EngineDebug.debug_state,
	debug_layer = EngineDebug.debug_layer,
	toSnapshot = EngineSnapshot.toSnapshot,
	applySnapshot = EngineSnapshot.applySnapshot,
}

return Engine
