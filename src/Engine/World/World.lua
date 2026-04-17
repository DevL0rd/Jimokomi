local Class = include("src/Engine/Core/Class.lua")
local WorldBounds = include("src/Engine/World/World/Bounds.lua")
local WorldVisibility = include("src/Engine/World/World/Visibility.lua")
local WorldTiles = include("src/Engine/World/World/Tiles.lua")
local WorldSampling = include("src/Engine/World/World/Sampling.lua")
local Raycast = include("src/Engine/World/Raycast.lua")
local Spawner = include("src/Engine/World/Spawner.lua")

local World = Class:new({
	_type = "World",
	layer = nil,
	raycast = nil,
	spawner = nil,

	init = function(self)
		self.raycast = Raycast:new({
			layer = self.layer,
		})
		self.spawner = Spawner:new({
			layer = self.layer,
			world = self,
		})
	end,

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
