local Class = include("src/classes/Class.lua")
local TileRegistry = include("src/classes/TileRegistry.lua")
local CollisionTileQueries = include("src/classes/collision/CollisionTileQueries.lua")
local CollisionShapeTests = include("src/classes/collision/CollisionShapeTests.lua")
local CollisionBroadphase = include("src/classes/collision/CollisionBroadphase.lua")
local CollisionResolver = include("src/classes/collision/CollisionResolver.lua")
local CollisionEvents = include("src/classes/collision/CollisionEvents.lua")

local Collision = Class:new({
	_type = "Collision",
	collision_passes = 1,
	wall_friction = 0.8,
	tile_size = 16,
	layer_bounds = nil,
	tile_registry = nil,
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
		})

		self.shape_tests = CollisionShapeTests
		self.broadphase = CollisionBroadphase.new({
			shape_tests = self.shape_tests,
		})
		self.resolver = CollisionResolver.new({
			collision_passes = self.collision_passes,
			wall_friction = self.wall_friction,
		})
		self.events = CollisionEvents

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
		self:resolveCollisions(entities, map_id)
		self:handleCollisionEvents(entities)
	end,
})

return Collision
