local Vector = include("src/Engine/Math/Vector.lua")
local EngineServices = include("src/Engine/Core/Engine/Services.lua")
local EngineLayers = include("src/Engine/Core/Engine/Layers.lua")
local EngineLoop = include("src/Engine/Core/Engine/Loop.lua")
local EngineDebug = include("src/Engine/Core/Engine/Debug.lua")
local EngineSnapshot = include("src/Engine/Core/Engine/Snapshot.lua")

local Engine = {
	layers = {},
	master_camera = nil,
	clock = nil,
	debug_buffer = nil,
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
	registerClass = EngineServices.registerClass,
	registerClasses = EngineServices.registerClasses,
	saveSnapshot = EngineServices.saveSnapshot,
	loadSnapshot = EngineServices.loadSnapshot,
	saveObject = EngineServices.saveObject,
	loadObject = EngineServices.loadObject,
	save = EngineServices.save,
	load = EngineServices.load,
	saveExists = EngineServices.saveExists,
	getSaveInfo = EngineServices.getSaveInfo,
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
