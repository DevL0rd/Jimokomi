local Class = include("src/classes/Class.lua")
local WorldQueryBounds = include("src/classes/world_query/WorldQueryBounds.lua")
local WorldQueryVisibility = include("src/classes/world_query/WorldQueryVisibility.lua")
local WorldQueryTiles = include("src/classes/world_query/WorldQueryTiles.lua")
local WorldQuerySampling = include("src/classes/world_query/WorldQuerySampling.lua")
local WorldQueryRules = include("src/classes/world_query/WorldQueryRules.lua")

local WorldQuery = Class:new({
	_type = "WorldQuery",
	layer = nil,

	init = function(self)
	end,

	getMapBounds = WorldQueryBounds.getMapBounds,
	clampBoundsToMap = WorldQueryBounds.clampBoundsToMap,
	getViewBounds = WorldQueryBounds.getViewBounds,
	getQueryBounds = WorldQueryBounds.getQueryBounds,
	getOffset = WorldQueryBounds.getOffset,
	getTileAt = WorldQueryTiles.getTileAt,
	isSolidAt = WorldQueryTiles.isSolidAt,
	isOnScreen = WorldQueryVisibility.isOnScreen,
	getRandomPointInBounds = WorldQuerySampling.getRandomPointInBounds,
	getRandomMapPosition = WorldQuerySampling.getRandomMapPosition,
	collectTilePositions = WorldQueryTiles.collectTilePositions,
	findSurfacePositionAtX = WorldQueryTiles.findSurfacePositionAtX,
	getRandomSurfacePosition = WorldQuerySampling.getRandomSurfacePosition,
	getRandomTilePosition = WorldQuerySampling.getRandomTilePosition,
	resolveRule = WorldQuerySampling.resolveRule,
})

WorldQuery.rules = WorldQueryRules.rules

return WorldQuery
