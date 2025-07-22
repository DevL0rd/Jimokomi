local Class = include("src/classes/Class.lua")
local Vector = include("src/classes/Vector.lua")
local Screen = include("src/classes/Screen.lua")
local Graphics = include("src/classes/Graphics.lua")
local Collision = include("src/classes/Collision.lua")
local Camera = include("src/classes/Camera.lua")

local Layer = Class:new({
    _type = "Layer",
    debug = false,
    entities = {},
    gravity = Vector:new({ y = 200 }),
    friction = 0.01,
    wall_friction = 0.05,
    collision_passes = 1, -- Reduced from 2 to prevent over-correction jitter
    running = false,
    layer_id = 0,
    physics_enabled = true,
    map_id = nil,
    tile_size = 16, -- Fixed: Match actual map tile size
    tile_properties = {
        [0] = { solid = false, name = "empty" },
        [41] = { solid = true, name = "ground" },
    },

    init = function(self)
        self.entities = {}

        -- Initialize collision system
        self.collision = Collision:new({
            collision_passes = self.collision_passes,
            wall_friction = self.wall_friction,
            tile_size = self.tile_size,
            world_bounds = { x = 0, y = 0, w = self.w or 1024, h = self.h or 512 }
        })

        -- Transfer tile properties to collision system
        for tile_id, properties in pairs(self.tile_properties) do
            self.collision:addTileType(tile_id, properties)
        end

        -- Initialize camera system
        self.camera = Camera:new({
            pos = Vector:new({ x = 0, y = 0 }),
            offset = Vector:new({ x = Screen.w / 2, y = Screen.h / 2 }),
            smoothing = 0.1,
            parallax_factor = Vector:new({ x = 1, y = 1 })
        })

        self.gfx = Graphics:new({
            camera = self.camera
        })
    end,

    update = function(self)
        if not self.running then
            return
        end

        for i = #self.entities, 1, -1 do
            local ent = self.entities[i]
            if not ent._delete then
                ent:update()
                if not ent.ignore_physics then
                    if not ent.ignore_gravity then
                        ent.vel:add(self.gravity, true)
                    end
                    if not ent.ignore_friction then
                        ent.vel:drag(self.friction, true)
                    end
                    ent.pos:add(ent.vel, true)
                end
            else
                if ent.unInit then
                    ent:unInit()
                end
                del(self.entities, ent)
            end
        end

        -- Update collision system IMMEDIATELY after physics
        self.collision:setWorldBounds(0, 0, self.w, self.h)
        self.collision:update(self.entities, self.map_id)

        -- Update camera after collision resolution
        self.camera:update()

        for _, ent in pairs(self.entities) do
            if ent.parent then
                ent.pos.x = ent.parent.pos.x + ent.init_pos.x
                ent.pos.y = ent.parent.pos.y + ent.init_pos.y
            end
        end
    end,

    draw = function(self)
        if not self.running then
            return
        end


        -- Draw grid for this layer if DEBUG is enabled
        if DEBUG then
            local grid_size = 16
            local view_top_left = self.camera:screenToWorld({ x = 0, y = 0 })
            local view_bottom_right = self.camera:screenToWorld({ x = Screen.w, y = Screen.h })

            local start_x = flr(view_top_left.x / grid_size) * grid_size
            local start_y = flr(view_top_left.y / grid_size) * grid_size

            -- Use different colors for different layers
            local grid_color = self.layer_id + 5 -- Different color per layer

            -- Vertical lines
            for x = start_x, view_bottom_right.x, grid_size do
                local start_pos = self.camera:worldToScreen({ x = x, y = view_top_left.y })
                local end_pos = self.camera:worldToScreen({ x = x, y = view_bottom_right.y })
                line(start_pos.x, start_pos.y, end_pos.x, end_pos.y, grid_color)
            end

            -- Horizontal lines
            for y = start_y, view_bottom_right.y, grid_size do
                local start_pos = self.camera:worldToScreen({ x = view_top_left.x, y = y })
                local end_pos = self.camera:worldToScreen({ x = view_bottom_right.x, y = y })
                line(start_pos.x, start_pos.y, end_pos.x, end_pos.y, grid_color)
            end
        end

        if self.map_id ~= nil then
            -- Apply camera transformation for map rendering
            local shake = self.camera:getShakeOffset()
            local cam_x = flr(self.camera.pos.x * self.camera.parallax_factor.x + shake.x)
            local cam_y = flr(self.camera.pos.y * self.camera.parallax_factor.y + shake.y)
            -- Set camera for map rendering (positive values to scroll map opposite to camera)
            camera(cam_x, cam_y)
            map(nil, 0, 0)
            camera() -- Reset camera
            -- Debug: Render tile IDs on each visible tile
            if DEBUG then
                local view_top_left = self.camera:screenToWorld({ x = 0, y = 0 })
                local view_bottom_right = self.camera:screenToWorld({ x = Screen.w, y = Screen.h })
                local tile_size = self.tile_size
                local start_tile_x = flr(view_top_left.x / tile_size)
                local end_tile_x = ceil(view_bottom_right.x / tile_size)
                local start_tile_y = flr(view_top_left.y / tile_size)
                local end_tile_y = ceil(view_bottom_right.y / tile_size)
                for tx = start_tile_x, end_tile_x do
                    for ty = start_tile_y, end_tile_y do
                        local tile_id = mget(tx, ty)
                        if tile_id ~= 0 then
                            local world_x = tx * tile_size + tile_size / 2
                            local world_y = ty * tile_size + tile_size / 2
                            local screen_pos = self.camera:worldToScreen({ x = world_x, y = world_y })
                            if screen_pos.x >= 0 and screen_pos.x < Screen.w and screen_pos.y >= 0 and screen_pos.y < Screen.h then
                                print(tile_id, screen_pos.x - 6, screen_pos.y - 4, 7)
                            end
                        end
                    end
                end
            end
        end

        for _, ent in pairs(self.entities) do
            ent:draw()
        end
    end,
    add = function(self, ent)
        add(self.entities, ent)
        ent.debug = ent.debug or self.debug
        ent.world = self
    end,

    start = function(self)
        self.running = true
    end,

    stop = function(self)
        self.running = false
    end,

    setMap = function(self, map_id)
        self.map_id = map_id
    end,
})

return Layer
