local Class = include("src/Engine/Core/Class.lua")
local RaycastShapes = include("src/Engine/World/Raycast/Shapes.lua")
local RaycastTiles = include("src/Engine/World/Raycast/Tiles.lua")
local RaycastEntities = include("src/Engine/World/Raycast/Entities.lua")

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
