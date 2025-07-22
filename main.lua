--[[pod_format="raw",created="2025-07-21 21:49:41",modified="2025-07-22 08:16:46",revision=4]]
DEBUG = true
DEBUG_TEXT = ""

function random_float(min_val, max_val)
	return min_val + rnd(max_val - min_val)
end

function random_int(min_val, max_val)
	return flr(min_val + rnd(max_val - min_val + 1))
end

function round(num, decimal_places)
	if decimal_places == nil then
		return flr(num + 0.5)
	else
		local factor = 10 ^ decimal_places
		return flr(num * factor + 0.5) / factor
	end
end

local Engine = include("src/classes/Engine.lua")
local JIJI = include("src/ent/JIJI.lua")
local Vector = include("src/classes/Vector.lua")



function _init()
	local layer = Engine:createLayer(0, true, 0)

	local jiji = JIJI:new({
		world = layer,
		pos = Vector:new({ x = 200, y = 100 })
	})
	layer.camera:setTarget(jiji)
	Engine:start()
end

function _update()
	Engine:update()
end

function _draw()
	Engine:draw()
	print(DEBUG_TEXT, 1, 1)
end
