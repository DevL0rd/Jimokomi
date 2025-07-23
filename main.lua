--[[pod_format="raw",created="2025-07-21 21:49:41",modified="2025-07-22 08:16:46",revision=4]]
DEBUG = true
include("src/polyfills.lua")
local Engine = include("src/classes/Engine.lua")
local JIJI = include("src/ent/JIJI.lua")
local CoCo = include("src/ent/CoCo.lua")
local Vector = include("src/classes/Vector.lua")



function _init()
	local layer = Engine:createLayer(0, true, 0)
	local jiji = JIJI:new({
		layer = layer,
		pos = Vector:new({ x = 420, y = 220 })
	})
	local coco = CoCo:new({
		layer = layer,
		pos = Vector:new({ x = 470, y = 328 })
	})
	layer.camera:setTarget(jiji)
	Engine:start()
end

function _update()
	Engine:update()
end

function _draw()
	Engine:draw()
end
