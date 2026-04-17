local Class = include("src/Engine/Core/Class.lua")
local TileRegistry = include("src/Engine/World/TileRegistry.lua")
local CollisionTileQueries = include("src/Engine/Physics/Collision/TileQueries.lua")
local CollisionShapeTests = include("src/Engine/Physics/Collision/ShapeTests.lua")
local CollisionBroadphase = include("src/Engine/Physics/Collision/Broadphase.lua")
local CollisionResolver = include("src/Engine/Physics/Collision/Resolver.lua")
local CollisionEvents = include("src/Engine/Physics/Collision/Events.lua")

local Collision = Class:new({
	_type = "Collision",
	collision_passes = 1,
	wall_friction = 0.8,
	tile_size = 16,
	layer_bounds = nil,
	tile_registry = nil,
	profiler = nil,
	tile_queries = nil,
	shape_tests = nil,
	broadphase = nil,
	resolver = nil,
	events = nil,

	init = function(self)
		if not self.tile_registry then
			self.tile_registry = TileRegistry:new()
		end

		self.tile_queries = CollisionTileQueries.new({
			tile_size = self.tile_size,
			layer_bounds = self.layer_bounds or { x = 0, y = 0, w = 1024, h = 512 },
			tile_registry = self.tile_registry,
			profiler = self.profiler,
		})

		self.shape_tests = CollisionShapeTests
		self.broadphase = CollisionBroadphase.new({
			shape_tests = self.shape_tests,
			profiler = self.profiler,
		})
		self.resolver = CollisionResolver.new({
			collision_passes = self.collision_passes,
			wall_friction = self.wall_friction,
			profiler = self.profiler,
		})
		self.events = {
			handleCollisionEvents = function(_, entities)
				return CollisionEvents.handleCollisionEvents(self.events, entities)
			end,
			profiler = self.profiler,
		}

		self.tile_registry:applyToCollision(self)
	end,

	addTileType = function(self, tile_id, properties)
		return self.tile_queries:addTileType(tile_id, properties)
	end,

	getTileIDAt = function(self, x, y, map_id)
		return self.tile_queries:getTileIDAt(x, y, map_id)
	end,

	getTileProperties = function(self, tile_id)
		return self.tile_queries:getTileProperties(tile_id)
	end,

	isSolidTile = function(self, tile_id)
		return self.tile_queries:isSolidTile(tile_id)
	end,

	setLayerBounds = function(self, x, y, w, h)
		return self.tile_queries:setLayerBounds(x, y, w, h)
	end,

	collectCollisions = function(self, entities, map_id)
		return self.broadphase:collectCollisions(entities, map_id, self.tile_queries)
	end,

	processCollisions = function(self, entities)
		return self.resolver:processCollisions(entities)
	end,

	resolveCollisions = function(self, entities, map_id)
		return self.resolver:resolveCollisions(entities, map_id, self.broadphase, self.tile_queries)
	end,

	handleCollisionEvents = function(self, entities)
		return self.events:handleCollisionEvents(entities)
	end,

	update = function(self, entities, map_id)
		local profiler = self.profiler
		if profiler then
			profiler:observe("collision.entities", #entities)
		end
		self:resolveCollisions(entities, map_id)
		self:handleCollisionEvents(entities)
	end,
})

return Collision
