local Engine = include("src/Engine/Core/Engine.lua")
local Glider = include("src/Game/Actors/Creatures/Glider/Glider.lua")
local Party = include("src/Game/Actors/Creatures/Glider/Party.lua")
local Hud = include("src/Game/Actors/Creatures/Glider/Hud.lua")
local Fly = include("src/Game/Actors/Creatures/Fly.lua")
local Worm = include("src/Game/Actors/Creatures/Worm.lua")
local Cherry = include("src/Game/Actors/Items/Food/Cherry.lua")
local Water = include("src/Game/Effects/Materials/Water.lua")
local SpaceBallSpawner = include("src/Game/Actors/Stress/SpaceBallSpawner.lua")
local Vector = include("src/Engine/Math/Vector.lua")

local runtime_error_path = "/appdata/jimokomi_runtime_error.txt"
local perf_history_path = "/appdata/jimokomi_perf_history.txt"
local perf_summary_path = "/appdata/jimokomi_perf_windows.txt"
local GAME_DEBUG = false
local GAME_PERF_PANEL = true

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

local function clear_perf_logs()
	if fstat(perf_history_path) == "file" then
		rm(perf_history_path)
	end
	if fstat(perf_summary_path) == "file" then
		rm(perf_summary_path)
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
	Engine.debug = GAME_DEBUG
	Engine.profiler_enabled = true
	Engine.debug_overlay_enabled = GAME_DEBUG
	Engine.performance_panel_enabled = GAME_PERF_PANEL
	Engine.detailed_entity_profiling_enabled = GAME_DEBUG
	Engine.debug_guides_enabled = false
	Engine.render_cache_enabled = true
	Engine.render_cache_mode = 3
	Engine.render_cache_tag_modes = {}
	Engine.debug_text_cache_enabled = true
	Engine.debug_shape_cache_enabled = true
	Engine.auto_toggle_debug_text_cache = false
	Engine.debug_text_cache_toggle_interval_ms = 10000
	Engine.auto_toggle_debug_shape_cache = false
	Engine.debug_shape_cache_toggle_interval_ms = 10000
	Engine:ensureServices()
	Engine.w = 16 * 64
	Engine.h = 16 * 32
	Engine.fill_color = 1
	Engine.stroke_color = 5
	local layer = Engine:createLayer(0, true, 0)
	layer.debug = GAME_DEBUG
	layer.debug_tile_labels = false
	layer.gfx:setRenderCacheMode(Engine.render_cache_mode or (Engine.render_cache_enabled ~= false and 3 or 2))
	local world = layer.world
	local party = Party:new({
		layer = layer,
	})
	runtime.party = party
	runtime.hud = Hud:new({
		party = party,
		gfx = layer.gfx,
	})

	local glider_specs = {
		{ name = "coco", x = 388, y = 220 },
		{ name = "mimi", x = 420, y = 220 },
		{ name = "jiji", x = 452, y = 220 },
		{ name = "momo", x = 484, y = 220 },
	}
	for i = 1, #glider_specs do
		local spec = glider_specs[i]
		local glider = world:spawn(Glider, {
			layer = layer,
			display_name = spec.name,
			pos = Vector:new({ x = spec.x, y = spec.y }),
		})
		party:addPlayer(glider)
	end
	party:setActiveIndex(3)

	world:spawnMany(6, Fly, { layer = layer })
	world:spawnMany(5, Worm, { layer = layer })
	world:spawnMany(8, Cherry, { layer = layer })
	world:spawn(Water, {
		layer = layer,
		debug = false,
		pos = Vector:new({ x = Engine.w * 0.5, y = Engine.h * 0.5 }),
		shape = {
			kind = "rect",
			w = 96,
			h = 56,
		},
	})
	world:spawn(SpaceBallSpawner, {
		layer = layer,
		pos = Vector:new({ x = Engine.w * 0.5, y = Engine.h * 0.5 }),
		spawn_interval_ms = 50,
		spawn_per_tick = 3,
		max_space_balls = 20,
		launch_speed = 125,
		upward_bias = 64,
	})
	Engine:start()
end

function Game.install()
	function _init()
		clear_runtime_error()
		clear_perf_logs()
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
