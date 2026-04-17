local Engine = include("src/Engine/Core/Engine.lua")
local Player = include("src/Game/Actors/Player/Player.lua")
local Party = include("src/Game/Actors/Player/Party.lua")
local Hud = include("src/Game/Actors/Player/Hud.lua")
local Fly = include("src/Game/Actors/Creatures/Fly.lua")
local Worm = include("src/Game/Actors/Creatures/Worm.lua")
local Cherry = include("src/Game/Actors/Items/Food/Cherry.lua")
local Vector = include("src/Engine/Math/Vector.lua")

local runtime_error_path = "/appdata/jimokomi_runtime_error.txt"

local Game = {}
local runtime = {
	party = nil,
	hud = nil,
}

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
	Engine.debug = false
	Engine.w = 16 * 64
	Engine.h = 16 * 32
	local layer = Engine:createLayer(0, true, 0)
	layer.debug = false
	local world = layer.world
	local party = Party:new({
		layer = layer,
	})
	runtime.party = party
	runtime.hud = Hud:new({
		party = party,
	})

	local player_specs = {
		{ name = "coco", x = 388, y = 220 },
		{ name = "mimi", x = 420, y = 220 },
		{ name = "jiji", x = 452, y = 220 },
		{ name = "momo", x = 484, y = 220 },
	}
	for i = 1, #player_specs do
		local spec = player_specs[i]
		local player = world:spawn(Player, {
			layer = layer,
			display_name = spec.name,
			pos = Vector:new({ x = spec.x, y = spec.y }),
		})
		party:addPlayer(player)
	end
	party:setActiveIndex(3)

	world:spawnMany(6, Fly, { layer = layer })
	world:spawnMany(5, Worm, { layer = layer })
	world:spawnMany(8, Cherry, { layer = layer })
	Engine:start()
end

function Game.install()
	function _init()
		clear_runtime_error()
		safe_phase("_init", boot_game)
	end

	function _update()
		safe_phase("_update", function()
			if runtime.party and runtime.party.handleControlSwapInput then
				runtime.party:handleControlSwapInput()
			end
			Engine:update()
		end)
	end

	function _draw()
		safe_phase("_draw", function()
			Engine:draw()
			if runtime.hud then
				runtime.hud:draw()
			end
		end)
	end
end

return Game
