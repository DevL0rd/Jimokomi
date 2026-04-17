local Class = include("src/classes/Class.lua")
local RaycastShapes = include("src/classes/raycast/RaycastShapes.lua")
local RaycastTiles = include("src/classes/raycast/RaycastTiles.lua")
local RaycastEntities = include("src/classes/raycast/RaycastEntities.lua")

local Raycast = Class:new({
	_type = "Raycast",
	layer = nil,

	rayToLayer = RaycastShapes.rayToLayer,
	rayToCircle = RaycastShapes.rayToCircle,
	rayToRect = RaycastShapes.rayToRect,
	rayToTiles = RaycastTiles.rayToTiles,
	canRayHitEntity = RaycastEntities.canRayHitEntity,
	rayToEntity = RaycastEntities.rayToEntity,
	cast = RaycastEntities.cast,
})

return Raycast
