local Vector = include("src/classes/Vector.lua")
local Rectangle = include("src/primitives/Rectangle.lua")
local Circle = include("src/primitives/Circle.lua")

local Collision = {
    new = function(self, config)
        config = config or {}
        local collision = setmetatable({}, { __index = self })

        -- Configuration
        collision.collision_passes = config.collision_passes or 3
        collision.wall_friction = config.wall_friction or 0.8
        collision.tile_size = config.tile_size or 16
        collision.world_bounds = config.world_bounds or { x = 0, y = 0, w = 1024, h = 512 }

        -- Tile properties for map collision
        collision.tile_properties = {}

        return collision
    end,

    -- Add tile type for map collision detection
    addTileType = function(self, tile_id, properties)
        self.tile_properties[tile_id] = properties
    end,

    -- Get tile at world coordinates
    getTileAt = function(self, x, y, map_id)
        if map_id == nil then return 0 end
        local tile_x = flr(x / self.tile_size)
        local tile_y = flr(y / self.tile_size)
        return mget(tile_x, tile_y)
    end,

    -- Check if a tile is solid
    isSolidTile = function(self, tile_id)
        local properties = self.tile_properties[tile_id]
        return properties and properties.solid or false
    end,

    -- Get tile properties
    getTileProperties = function(self, tile_id)
        return self.tile_properties[tile_id] or { solid = false, name = "unknown" }
    end,

    -- Check collision between circle and world bounds
    circleToWorld = function(self, circle)
        local bounds = self.world_bounds
        local push_x = 0
        local push_y = 0

        if circle.pos.x - circle.r < bounds.x then
            push_x = (bounds.x + circle.r) - circle.pos.x
        elseif circle.pos.x + circle.r > bounds.x + bounds.w then
            push_x = (bounds.x + bounds.w) - (circle.pos.x + circle.r)
        end

        if circle.pos.y - circle.r < bounds.y then
            push_y = (bounds.y + circle.r) - circle.pos.y
        elseif circle.pos.y + circle.r > bounds.y + bounds.h then
            push_y = (bounds.y + bounds.h) - (circle.pos.y + circle.r)
        end

        if push_x ~= 0 or push_y ~= 0 then
            return Vector:new({ x = push_x, y = push_y })
        else
            return false
        end
    end,

    -- Check collision between rectangle and world bounds
    rectToWorld = function(self, rect)
        local bounds = self.world_bounds
        local push_x = 0
        local push_y = 0

        local left = rect.pos.x - rect.w / 2
        local right = rect.pos.x + rect.w / 2
        local top = rect.pos.y - rect.h / 2
        local bottom = rect.pos.y + rect.h / 2

        if left < bounds.x then
            push_x = bounds.x - left
        elseif right > bounds.x + bounds.w then
            push_x = (bounds.x + bounds.w) - right
        end

        if top < bounds.y then
            push_y = bounds.y - top
        elseif bottom > bounds.y + bounds.h then
            push_y = (bounds.y + bounds.h) - bottom
        end

        if push_x ~= 0 or push_y ~= 0 then
            return Vector:new({ x = push_x, y = push_y })
        else
            return false
        end
    end,

    -- Check collision between entity and map tiles
    checkMapCollision = function(self, entity, map_id)
        if map_id == nil then return false end

        local push_x = 0
        local push_y = 0

        if entity:is_a(Rectangle) then
            local left = entity.pos.x - entity.w / 2
            local right = entity.pos.x + entity.w / 2
            local top = entity.pos.y - entity.h / 2
            local bottom = entity.pos.y + entity.h / 2

            local tl = self:getTileAt(left, top, map_id)
            local tr = self:getTileAt(right, top, map_id)
            local bl = self:getTileAt(left, bottom, map_id)
            local br = self:getTileAt(right, bottom, map_id)

            if self:isSolidTile(tl) or self:isSolidTile(tr) or self:isSolidTile(bl) or self:isSolidTile(br) then
                local tile_x = flr(entity.pos.x / self.tile_size) * self.tile_size
                local tile_y = flr(entity.pos.y / self.tile_size) * self.tile_size

                local overlap_left = (tile_x + self.tile_size) - left
                local overlap_right = right - tile_x
                local overlap_top = (tile_y + self.tile_size) - top
                local overlap_bottom = bottom - tile_y

                if overlap_left < overlap_right and overlap_left < overlap_top and overlap_left < overlap_bottom then
                    push_x = overlap_left
                elseif overlap_right < overlap_top and overlap_right < overlap_bottom then
                    push_x = -overlap_right
                elseif overlap_top < overlap_bottom then
                    push_y = overlap_top
                else
                    push_y = -overlap_bottom
                end
            end
        elseif entity:is_a(Circle) then
            local tile = self:getTileAt(entity.pos.x, entity.pos.y, map_id)
            if self:isSolidTile(tile) then
                local tile_x = flr(entity.pos.x / self.tile_size) * self.tile_size
                local tile_y = flr(entity.pos.y / self.tile_size) * self.tile_size
                local tile_center_x = tile_x + self.tile_size / 2
                local tile_center_y = tile_y + self.tile_size / 2

                local dx = entity.pos.x - tile_center_x
                local dy = entity.pos.y - tile_center_y
                local dist = sqrt(dx * dx + dy * dy)

                if dist < entity.r + self.tile_size / 2 then
                    local push_dist = (entity.r + self.tile_size / 2) - dist
                    push_x = (dx / dist) * push_dist
                    push_y = (dy / dist) * push_dist
                end
            end
        end

        if push_x ~= 0 or push_y ~= 0 then
            return Vector:new({ x = push_x, y = push_y })
        else
            return false
        end
    end,

    -- Collect all collisions for entities in a given entity list
    collectCollisions = function(self, entities, map_id)
        for index, entity in pairs(entities) do
            if entity.ignore_physics or entity.ignore_collisions then
                goto skip
            end

            entity.collisions = {}

            -- Check world bounds collision
            local world_collision = nil
            if entity:is_a(Rectangle) then
                world_collision = self:rectToWorld(entity)
            elseif entity:is_a(Circle) then
                world_collision = self:circleToWorld(entity)
            end

            if world_collision then
                add(entity.collisions, {
                    object = "world",
                    vector = world_collision
                })
            end

            -- Check map collision
            local map_collision = self:checkMapCollision(entity, map_id)
            if map_collision then
                add(entity.collisions, {
                    object = "map",
                    vector = map_collision
                })
            end

            ::skip::
        end
    end,

    -- Process collisions and apply physics responses
    processCollisions = function(self, entities)
        for index, entity in pairs(entities) do
            if entity.ignore_physics or entity.ignore_collisions or not entity.collisions then
                goto skip
            end

            local total_push = Vector:new({ x = 0, y = 0 })
            local has_x_collision = false
            local has_y_collision = false

            for _, collision in pairs(entity.collisions) do
                total_push:add(collision.vector)
                if collision.vector.x ~= 0 then
                    has_x_collision = true
                end
                if collision.vector.y ~= 0 then
                    has_y_collision = true
                end
            end

            if total_push.x ~= 0 or total_push.y ~= 0 then
                entity.pos:add(total_push)

                -- Apply wall friction if entity has velocity
                if entity.vel then
                    entity.vel:drag(self.wall_friction, false)

                    if has_x_collision then
                        entity.vel.x = 0
                    end
                    if has_y_collision then
                        entity.vel.y = 0
                    end
                end
            end

            ::skip::
        end
    end,

    -- Resolve all collisions for a frame (multiple passes for stability)
    resolveCollisions = function(self, entities, map_id)
        for i = 1, self.collision_passes do
            self:collectCollisions(entities, map_id)
            self:processCollisions(entities)
        end
    end,

    -- Handle collision events (call entity collision callbacks)
    handleCollisionEvents = function(self, entities)
        for index, entity in pairs(entities) do
            if entity.collisions and #entity.collisions > 0 and entity.on_collision then
                for _, collision in pairs(entity.collisions) do
                    entity:on_collision(collision.object, collision.vector)
                end
            end
        end
    end,

    -- Set world bounds for collision detection
    setWorldBounds = function(self, x, y, w, h)
        self.world_bounds = { x = x, y = y, w = w, h = h }
    end,

    -- Update collision system (full collision resolution cycle)
    update = function(self, entities, map_id)
        self:resolveCollisions(entities, map_id)
        self:handleCollisionEvents(entities)
    end
}

return Collision
