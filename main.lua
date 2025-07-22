--[[pod_format="raw",created="2025-07-21 21:49:41",modified="2025-07-21 22:41:39",revision=3]]
DEBUG = true
DEBUG_TEXT = ""
include "src/engine.lua"
include "src/ent/jiji.lua"

function _init()
	WORLD = Engine:createLayer(0, true, 0)
	local jiji = Jiji:new({
		world = WORLD,
		pos = Vector:new({ x = 200, y = 100 })
	})
	WORLD.camera:setTarget(jiji)
	Engine:start()
end

function _update()
	Engine:update()
end

function _draw()
	Engine:draw()
	print(DEBUG_TEXT, 1, 1)
end
