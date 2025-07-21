DEBUG = true
DEBUG_TEXT = ""
include "src/engine.lua"
include "src/ent/jiji.lua"

function _init()
	WORLD = Engine:createLayer(0)
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
