--[[pod_format="raw",created="2025-07-21 21:49:41",modified="2025-07-22 08:16:46",revision=4]]
DEBUG = false
include("src/polyfills.lua")
local Engine = include("src/classes/Engine.lua")
local JIJI = include("src/ent/jiji.lua")
local CoCo = include("src/ent/CoCo.lua")
local Fly = include("src/ent/Fly.lua")
local Worm = include("src/ent/Worm.lua")
local Cherry = include("src/ent/Cherry.lua")
local Vector = include("src/classes/Vector.lua")

local function boot_game()
	local layer = Engine:createLayer(0, true, 0)
	local jiji = JIJI:new({
		layer = layer,
		pos = Vector:new({ x = 420, y = 220 })
	})
	local coco = CoCo:new({
		layer = layer,
		pos = Vector:new({ x = 470, y = 328 })
	})
	for i = 1, 6 do
		Fly:new({ layer = layer })
	end
	for i = 1, 5 do
		Worm:new({ layer = layer })
	end
	for i = 1, 8 do
		Cherry:new({ layer = layer })
	end
	layer.camera:setTarget(jiji)
	Engine:start()
end



function _init()
	boot_game()
end

function _update()
	Engine:update()
end

function _draw()
	Engine:draw()
end
