--[[pod_format="raw",created="2025-07-22 09:38:12",modified="2025-07-22 09:38:12",revision=0]]
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
        collision.tile_size = config.tile_size or 16 -- Fixed: Default to 16 to match Picotron maps
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
        if map_id == nil then
            return 0
        end

        local tile_x = flr(x / self.tile_size)
        local tile_y = flr(y / self.tile_size)

        -- In Picotron, mget might need the map to be active or use map coordinates
        -- Try direct mget first
        local tile_id = mget(tile_x, tile_y)

        -- Handle nil tile_id (out of bounds)
        if tile_id == nil then
            return 0
        end

        return tile_id
    end,

    -- Check if a tile is solid
    isSolidTile = function(self, tile_id)
        if tile_id == nil then
            return false
        end

        local properties = self.tile_properties[tile_id]
        local is_solid = properties and properties.solid or false

        return is_solid
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

            -- Check all tiles that the rectangle overlaps
            local left_tile = flr(left / self.tile_size)
            local right_tile = flr(right / self.tile_size)
            local top_tile = flr(top / self.tile_size)
            local bottom_tile = flr(bottom / self.tile_size)

            local collided_tiles = {}

            for tile_x = left_tile, right_tile do
                for tile_y = top_tile, bottom_tile do
                    local tile_id = self:getTileAt(tile_x * self.tile_size, tile_y * self.tile_size, map_id)
                    if self:isSolidTile(tile_id) then
                        add(collided_tiles, { x = tile_x, y = tile_y })
                    end
                end
            end

            -- Calculate push vector for each collided tile
            for _, tile in pairs(collided_tiles) do
                local tile_left = tile.x * self.tile_size
                local tile_right = (tile.x + 1) * self.tile_size
                local tile_top = tile.y * self.tile_size
                local tile_bottom = (tile.y + 1) * self.tile_size

                -- Calculate overlaps with more precision
                local overlap_left = tile_right - left
                local overlap_right = right - tile_left
                local overlap_top = tile_bottom - top
                local overlap_bottom = bottom - tile_top

                -- Only process if there's actually an overlap
                if overlap_left > 0 and overlap_right > 0 and overlap_top > 0 and overlap_bottom > 0 then
                    -- Find the smallest overlap to determine push direction
                    local min_overlap = min(min(overlap_left, overlap_right), min(overlap_top, overlap_bottom))

                    -- Add separation distance to ensure complete uncolliding
                    local separation = 0.5

                    if min_overlap == overlap_left then
                        -- Push right (out of left side of tile)
                        local push_amount = overlap_left + separation
                        push_x = max(push_x, push_amount)
                    elseif min_overlap == overlap_right then
                        -- Push left (out of right side of tile)
                        local push_amount = -(overlap_right + separation)
                        push_x = min(push_x, push_amount)
                    elseif min_overlap == overlap_top then
                        -- Push down (out of top of tile)
                        local push_amount = overlap_top + separation
                        push_y = max(push_y, push_amount)
                    else
                        -- Push up (out of bottom of tile)
                        local push_amount = -(overlap_bottom + separation)
                        push_y = min(push_y, push_amount)
                    end
                end
            end
        elseif entity:is_a(Circle) then
            -- For circles, check tiles in a radius around the circle
            local center_tile_x = flr(entity.pos.x / self.tile_size)
            local center_tile_y = flr(entity.pos.y / self.tile_size)
            local radius_in_tiles = ceil(entity.r / self.tile_size)

            for dx = -radius_in_tiles, radius_in_tiles do
                for dy = -radius_in_tiles, radius_in_tiles do
                    local tile_x = center_tile_x + dx
                    local tile_y = center_tile_y + dy
                    local tile_id = self:getTileAt(tile_x * self.tile_size, tile_y * self.tile_size, map_id)

                    if self:isSolidTile(tile_id) then
                        -- Calculate distance from circle center to closest point on tile
                        local tile_left = tile_x * self.tile_size
                        local tile_right = (tile_x + 1) * self.tile_size
                        local tile_top = tile_y * self.tile_size
                        local tile_bottom = (tile_y + 1) * self.tile_size

                        local closest_x = max(tile_left, min(entity.pos.x, tile_right))
                        local closest_y = max(tile_top, min(entity.pos.y, tile_bottom))

                        local dx = entity.pos.x - closest_x
                        local dy = entity.pos.y - closest_y
                        local distance = sqrt(dx * dx + dy * dy)

                        if distance < entity.r then
                            local push_distance = entity.r - distance
                            if distance > 0 then
                                push_x = push_x + (dx / distance) * push_distance
                                push_y = push_y + (dy / distance) * push_distance
                            else
                                -- Circle center is inside tile, push in direction of least penetration
                                local push_left = entity.pos.x - tile_left
                                local push_right = tile_right - entity.pos.x
                                local push_up = entity.pos.y - tile_top
                                local push_down = tile_bottom - entity.pos.y

                                local min_push = min(min(push_left, push_right), min(push_up, push_down))
                                if min_push == push_left then
                                    push_x = push_x - push_distance
                                elseif min_push == push_right then
                                    push_x = push_x + push_distance
                                elseif min_push == push_up then
                                    push_y = push_y - push_distance
                                else
                                    push_y = push_y + push_distance
                                end
                            end
                        end
                    end
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

            -- Separate collisions by type for better resolution order
            local map_collisions = {}
            local world_collisions = {}
            local entity_collisions = {}

            for _, collision in pairs(entity.collisions) do
                if collision.object == "map" then
                    add(map_collisions, collision)
                elseif collision.object == "world" then
                    add(world_collisions, collision)
                else
                    add(entity_collisions, collision)
                end
            end

            local has_x_collision = false
            local has_y_collision = false

            -- Process map collisions first (most important for smooth movement)
            for _, collision in pairs(map_collisions) do
                -- Apply collision resolution immediately for smoother response
                entity.pos:add(collision.vector)

                if collision.vector.x ~= 0 then
                    has_x_collision = true
                end
                if collision.vector.y ~= 0 then
                    has_y_collision = true
                end
            end

            -- Then world collisions
            for _, collision in pairs(world_collisions) do
                entity.pos:add(collision.vector)

                if collision.vector.x ~= 0 then
                    has_x_collision = true
                end
                if collision.vector.y ~= 0 then
                    has_y_collision = true
                end
            end

            -- Finally entity collisions
            for _, collision in pairs(entity_collisions) do
                entity.pos:add(collision.vector)

                if collision.vector.x ~= 0 then
                    has_x_collision = true
                end
                if collision.vector.y ~= 0 then
                    has_y_collision = true
                end
            end

            -- Apply velocity changes after position correction
            if entity.vel and (has_x_collision or has_y_collision) then
                -- Apply wall friction
                entity.vel:drag(self.wall_friction, true)

                -- Stop velocity in collision directions
                if has_x_collision then
                    entity.vel.x = 0
                end
                if has_y_collision then
                    entity.vel.y = 0
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
