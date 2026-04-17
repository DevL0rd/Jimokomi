--[[pod_format="raw",created="2025-07-21 21:49:41",modified="2025-07-22 08:16:46",revision=4]]
DEBUG = false
include("src/polyfills.lua")
local Engine = include("src/classes/Engine.lua")
local Jiji = include("src/entities/Jiji.lua")
local CoCo = include("src/entities/creatures/CoCo.lua")
local Fly = include("src/entities/creatures/Fly.lua")
local Worm = include("src/entities/creatures/Worm.lua")
local Cherry = include("src/entities/food/Cherry.lua")
local Vector = include("src/classes/Vector.lua")
local runtime_error_path = "/appdata/jimokomi_runtime_error.txt"

local function clear_runtime_error()
	if fstat(runtime_error_path) == "file" then
		rm(runtime_error_path)
	end
end

local function write_runtime_error(phase, err)
	local lines = {
		"Jimokomi runtime error",
		"phase: " .. phase,
		"time: " .. tostr(time()),
		"error: " .. tostr(err),
	}
	local message = lines[1] .. "\n" .. lines[2] .. "\n" .. lines[3] .. "\n" .. lines[4]
	pcall(function()
		printh(message)
	end)
	pcall(function()
		store(runtime_error_path, message)
	end)
end

local function safe_phase(phase, fn)
	local ok, err = pcall(fn)
	if ok then
		return
	end
	write_runtime_error(phase, err)
	error(err)
end

local function boot_game()
	Engine:ensureServices()
	local world_config = Engine.assets:getWorldConfig()
	local layer_config = world_config.layer or {}
	local spawns = world_config.spawns or {}
	local layer = Engine:createLayer(
		layer_config.id or 0,
		layer_config.physics_enabled ~= false,
		layer_config.map_id or 0
	)
	local jiji = Jiji:new({
		layer = layer,
		pos = Vector:new(spawns.player or { x = 420, y = 220 })
	})
	layer:setPlayer(jiji)
	local coco = CoCo:new({
		layer = layer,
		pos = Vector:new(spawns.coco or { x = 470, y = 328 })
	})
	for i = 1, spawns.Fly or 6 do
		Fly:new({ layer = layer })
	end
	for i = 1, spawns.Worm or 5 do
		Worm:new({ layer = layer })
	end
	for i = 1, spawns.Cherry or 8 do
		Cherry:new({ layer = layer })
	end
	layer.camera:setTarget(jiji)
	Engine:start()
end



function _init()
	clear_runtime_error()
	safe_phase("_init", boot_game)
end

function _update()
	safe_phase("_update", function()
		Engine:update()
	end)
end

function _draw()
	safe_phase("_draw", function()
		Engine:draw()
	end)
end
