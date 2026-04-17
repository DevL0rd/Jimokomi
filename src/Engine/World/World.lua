local Class = include("src/Engine/Core/Class.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local WorldBounds = include("src/Engine/World/World/Bounds.lua")
local WorldSenses = include("src/Engine/World/World/Senses.lua")
local WorldVisibility = include("src/Engine/World/World/Visibility.lua")
local WorldTiles = include("src/Engine/World/World/Tiles.lua")
local WorldSampling = include("src/Engine/World/World/Sampling.lua")
local Raycast = include("src/Engine/World/Raycast.lua")
local Spawner = include("src/Engine/World/Spawner.lua")
local Pathfinder = include("src/Engine/World/Pathfinder.lua")

local World = Class:new({
	_type = "World",
	layer = nil,
	raycast = nil,
	spawner = nil,
	pathfinder = nil,
	recent_sounds = nil,
	sound_memory_ms = 1200,
	path_rebuilds_per_frame = 1,
	perception_checks_per_frame = 3,
	path_budget_frame = nil,
	path_budget_remaining = 0,
	perception_budget_frame = nil,
	perception_budget_remaining = 0,

	init = function(self)
		self.raycast = Raycast:new({
			layer = self.layer,
		})
		self.spawner = Spawner:new({
			layer = self.layer,
			world = self,
		})
		self.pathfinder = Pathfinder:new({
			world = self,
		})
		self.recent_sounds = {}
		self:resetPathBudget()
		self:resetPerceptionBudget()
	end,

	getTileSize = function(self)
		local layer = self.layer
		return layer and layer.tile_size or 16
	end,
	getTileBounds = function(self)
		local layer = self.layer
		local tile_size = self:getTileSize()
		if not layer then
			return nil
		end
		return {
			min_x = 0,
			min_y = 0,
			max_x = max(0, flr((layer.w - 1) / tile_size)),
			max_y = max(0, flr((layer.h - 1) / tile_size)),
		}
	end,
	clampTile = function(self, tile_x, tile_y)
		local bounds = self:getTileBounds()
		if not bounds then
			return tile_x, tile_y
		end
		return mid(bounds.min_x, tile_x, bounds.max_x), mid(bounds.min_y, tile_y, bounds.max_y)
	end,
	worldToTile = function(self, pos)
		if not pos then
			return 0, 0
		end
		local tile_size = self:getTileSize()
		local tile_x = flr(pos.x / tile_size)
		local tile_y = flr(pos.y / tile_size)
		return self:clampTile(tile_x, tile_y)
	end,
	tileToWorld = function(self, tile_x, tile_y)
		local tile_size = self:getTileSize()
		return Vector:new({
			x = tile_x * tile_size + tile_size * 0.5,
			y = tile_y * tile_size + tile_size * 0.5,
		})
	end,
	getFrameId = WorldSenses.getFrameId,
	resetPathBudget = WorldSenses.resetPathBudget,
	resetPerceptionBudget = WorldSenses.resetPerceptionBudget,
	consumePathBudget = WorldSenses.consumePathBudget,
	consumePerceptionBudget = WorldSenses.consumePerceptionBudget,
	hasLineOfSight = WorldSenses.hasLineOfSight,
	pruneSounds = WorldSenses.pruneSounds,
	emitSound = WorldSenses.emitSound,
	findNearestSound = WorldSenses.findNearestSound,

	getMapBounds = WorldBounds.getMapBounds,
	clampBoundsToMap = WorldBounds.clampBoundsToMap,
	getViewBounds = WorldBounds.getViewBounds,
	getQueryBounds = WorldBounds.getQueryBounds,
	getOffset = WorldBounds.getOffset,
	getTileAt = WorldTiles.getTileAt,
	isSolidAt = WorldTiles.isSolidAt,
	isOnScreen = WorldVisibility.isOnScreen,
	getRandomPointInBounds = WorldSampling.getRandomPointInBounds,
	getRandomMapPosition = WorldSampling.getRandomMapPosition,
	collectTilePositions = WorldTiles.collectTilePositions,
	findSurfacePositionAtX = WorldTiles.findSurfacePositionAtX,
	getRandomSurfacePosition = WorldSampling.getRandomSurfacePosition,
	getRandomTilePosition = WorldSampling.getRandomTilePosition,
	resolveRule = WorldSampling.resolveRule,
	findPath = function(self, start_node, goal_node, options)
		return self.pathfinder:findPath(start_node, goal_node, options)
	end,
	castRay = function(self, ray)
		return self.raycast:cast(ray)
	end,
	spawn = function(self, ObjectClass, params, rule, require_offscreen, radius)
		return self.spawner:spawn(ObjectClass, params, rule, require_offscreen, radius)
	end,
	spawnMany = function(self, count, ObjectClass, params, rule, require_offscreen, radius)
		return self.spawner:spawnMany(count, ObjectClass, params, rule, require_offscreen, radius)
	end,
})

return World
