local Engine = include("src/Engine/Core/Engine.lua")
local Player = include("src/Game/Actors/Player/Player.lua")
local CoCo = include("src/Game/Actors/Creatures/CoCo.lua")
local Fly = include("src/Game/Actors/Creatures/Fly.lua")
local Worm = include("src/Game/Actors/Creatures/Worm.lua")
local Cherry = include("src/Game/Actors/Items/Food/Cherry.lua")
local Vector = include("src/Engine/Math/Vector.lua")

local runtime_error_path = "/appdata/jimokomi_runtime_error.txt"

local Game = {}

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
	Engine.w = 16 * 64
	Engine.h = 16 * 32
	local layer = Engine:createLayer(0, true, 0)
	local world = layer.world

	local player = world:spawn(Player, {
		layer = layer,
		pos = Vector:new({ x = 420, y = 220 })
	})
	layer:setPlayer(player)

	world:spawn(CoCo, {
		layer = layer,
		pos = Vector:new({ x = 470, y = 328 })
	})

	world:spawnMany(6, Fly, { layer = layer })
	world:spawnMany(5, Worm, { layer = layer })
	world:spawnMany(8, Cherry, { layer = layer })

	layer.camera:setTarget(player)
	Engine:start()
end

function Game.install()
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
end

return Game
