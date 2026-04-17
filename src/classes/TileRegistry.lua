local Class = include("src/classes/Class.lua")
local UNKNOWN_TILE = { solid = false, name = "unknown" }

local TileRegistry = Class:new({
	_type = "TileRegistry",
	tiles = nil,

	init = function(self)
		self.tiles = self.tiles or {}
	end,

	register = function(self, tile_id, properties)
		self.tiles[tile_id] = properties
	end,

	registerMany = function(self, definitions)
		for tile_id, properties in pairs(definitions or {}) do
			self:register(tile_id, properties)
		end
	end,

	get = function(self, tile_id)
		return self.tiles[tile_id] or UNKNOWN_TILE
	end,

	isSolid = function(self, tile_id)
		local properties = self:get(tile_id)
		return properties and properties.solid or false
	end,

	applyToCollision = function(self, collision)
		for tile_id, properties in pairs(self.tiles) do
			collision:addTileType(tile_id, properties)
		end
	end,
})

return TileRegistry
