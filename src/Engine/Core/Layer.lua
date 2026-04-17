local Class = include("src/Engine/Core/Class.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local Screen = include("src/Engine/Core/Screen.lua")
local Graphics = include("src/Engine/Rendering/Graphics.lua")
local Collision = include("src/Engine/Physics/Collision.lua")
local Camera = include("src/Engine/Core/Camera.lua")
local World = include("src/Engine/World/World.lua")
local TileRegistry = include("src/Engine/World/TileRegistry.lua")
local EventBus = include("src/Engine/Core/EventBus.lua")
local Renderer = include("src/Engine/Core/Layer/Renderer.lua")
local Simulation = include("src/Engine/Core/Layer/Simulation.lua")
local LayerBuckets = include("src/Engine/Core/Layer/Buckets.lua")
local LayerObjects = include("src/Engine/Core/Layer/Objects.lua")
local LayerSnapshot = include("src/Engine/Core/Layer/Snapshot.lua")

local Layer = Class:new({
    _type = "Layer",
    debug = false,
    entities = {},
    attached_entities = {},
    physics_entities = {},
    collidable_entities = {},
    drawable_entities = {},
    gravity = Vector:new({ y = 200 }),
    friction = 0.5,
    wall_friction = 2,
    collision_passes = 1,
    running = false,
    layer_id = 0,
    physics_enabled = true,
    map_id = nil,
    player = nil,
    tile_size = 16, -- Fixed: Match actual map tile size

    init = function(self)
        self.entities = {}
        LayerBuckets.reset(self)
        self.tile_registry = TileRegistry:new()
        self.tile_registry:registerMany({
            [0] = { solid = false, name = "empty" },
            [23] = { solid = true, name = "ground" },
            [53] = { solid = true, name = "wood" },
            [54] = { solid = true, name = "wood" },
            [55] = { solid = true, name = "wood" },
            [63] = { solid = true, name = "wood" },
        })

        self.collision = Collision:new({
            collision_passes = self.collision_passes,
            wall_friction = self.wall_friction,
            tile_size = self.tile_size,
            layer_bounds = { x = 0, y = 0, w = self.w or 1024, h = self.h or 512 },
            tile_registry = self.tile_registry,
            profiler = self.engine and self.engine.profiler or nil,
        })

        self.camera = Camera:new({
            pos = Vector:new({ x = 0, y = 0 }),
            offset = Vector:new({ x = Screen.w / 2, y = Screen.h / 2 }),
            smoothing = 0.1,
            parallax_factor = Vector:new({ x = 1, y = 1 })
        })

        self.gfx = Graphics:new({
            camera = self.camera,
            profiler = self.engine and self.engine.profiler or nil,
        })

        self.world = World:new({
            layer = self
        })

        self.events = EventBus:new({
            parent_bus = self.engine and self.engine.events or nil
        })

        self.renderer = Renderer:new({
            layer = self
        })

        self.simulation = Simulation:new({
            layer = self
        })
    end,
    on = function(self, name, handler)
        return self.events:on(name, handler)
    end,
    once = function(self, name, handler)
        return self.events:once(name, handler)
    end,
    off = function(self, name, handler)
        return self.events:off(name, handler)
    end,
    emit = function(self, name, payload)
        return self.events:emit(name, payload, true)
    end,
    emitIfListening = function(self, name, payload_builder)
        if not self.events or not self.events.hasListeners or not self.events:hasListeners(name, true) then
            return nil
        end
        local payload = payload_builder and payload_builder() or nil
        return self.events:emit(name, payload, true)
    end,

    update = function(self)
        if not self.running then
            return
        end
        self.simulation:update()
    end,

    draw = function(self)
        if not self.running then
            return
        end
        self.renderer:draw()
    end,
    registerEntityBuckets = function(self, ent)
        return LayerBuckets.registerEntity(self, ent)
    end,
    unregisterEntityBuckets = function(self, ent)
        return LayerBuckets.unregisterEntity(self, ent)
    end,
    refreshAttachmentBucket = function(self, ent)
        return LayerBuckets.refreshAttachment(self, ent)
    end,
    refreshEntityBuckets = function(self, ent)
        return LayerBuckets.refreshEntity(self, ent)
    end,
    add = function(self, ent)
        return LayerObjects.add(self, ent)
    end,
    clearObjects = function(self)
        return LayerObjects.clear(self)
    end,

    start = function(self)
        self.running = true
    end,

    stop = function(self)
        self.running = false
    end,

    setMap = function(self, map_id)
        if self.map_id ~= map_id and self.collision and self.collision.tile_queries and self.collision.tile_queries.invalidateSolidRowSpans then
            self.collision.tile_queries:invalidateSolidRowSpans(map_id)
        end
        if self.map_id ~= map_id and self.world then
            self.world.sampling_cache = {}
        end
        if self.map_id ~= map_id and self.gfx and self.gfx.clearRenderCache then
            self.gfx:clearRenderCache()
        end
        self.map_id = map_id
    end,

    setPlayer = function(self, ent)
        self.player = ent
    end,

	getPlayer = function(self)
        return self.player
    end,
	getAssets = function(self)
        return nil
    end,
    toSnapshot = function(self)
        return LayerSnapshot.toSnapshot(self)
    end,
    applySnapshot = function(self, snapshot, registry)
        return LayerSnapshot.applySnapshot(self, snapshot, registry)
    end,
})

return Layer
