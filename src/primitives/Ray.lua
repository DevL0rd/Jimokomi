local Entity = include("src/primitives/Entity.lua")
local Vector = include("src/classes/Vector.lua")
local Timer = include("src/classes/Timer.lua")
local Circle = include("src/primitives/Circle.lua")
local Rectangle = include("src/primitives/Rectangle.lua")

local Ray = Entity:new({
    vec = Vector:new({ y = 1 }),
    length = 50,
    rate = 50,
    ignore_physics = true,
    init = function(self)
        Entity.init(self)
        self.init_pos = Vector:new({
            x = self.pos.x,
            y = self.pos.y
        })
        self.line_dist = self.length
        self.timer = Timer:new()
        self.cached_result = { nil, self.length }
        if self.parent then
            self.world = self.parent.world
        end
        if self.world then
            self.world:add(self)
        end
    end,
    rayToWorld = function(self, world_obj)
        local t_x_min, t_x_max
        local t_y_min, t_y_max

        if self.vec.x == 0 then
            if self.pos.x < world_obj.x or self.pos.x > world_obj.x + world_obj.w then
                return false
            end
            t_x_min = -32767
            t_x_max = 32767
        else
            t_x_min = (world_obj.x - self.pos.x) / self.vec.x
            t_x_max = (world_obj.x + world_obj.w - self.pos.x) / self.vec.x
            if t_x_min > t_x_max then t_x_min, t_x_max = t_x_max, t_x_min end
        end

        if self.vec.y == 0 then
            if self.pos.y < world_obj.y or self.pos.y > world_obj.y + world_obj.h then
                return false
            end
            t_y_min = -32767
            t_y_max = 32767
        else
            t_y_min = (world_obj.y - self.pos.y) / self.vec.y
            t_y_max = (world_obj.y + world_obj.h - self.pos.y) / self.vec.y
            if t_y_min > t_y_max then t_y_min, t_y_max = t_y_max, t_y_min end
        end

        local t_enter = max(t_x_min, t_y_min)
        local t_exit = min(t_x_max, t_y_max)

        if t_enter > t_exit or t_exit < 0 then
            return false
        end

        if t_enter < 0 then
            return t_exit
        else
            return t_enter
        end
    end,

    rayToCircle = function(self, circ_obj)
        local l = Vector:new({ x = circ_obj.pos.x - self.pos.x, y = circ_obj.pos.y - self.pos.y })
        local tca = l:dot(self.vec)

        if tca < 0 then return false end

        local d2 = l:dot(l) -
            tca *
            tca
        local r2 = circ_obj.r * circ_obj.r

        if d2 > r2 then return false end

        local thc = sqrt(r2 - d2)
        local t = tca - thc

        if t < 0 then
            t = tca + thc
        end

        return t
    end,

    rayToRect = function(self, rect_obj)
        local t_near_x, t_far_x
        local t_near_y, t_far_y

        local left = rect_obj.pos.x - rect_obj.w / 2
        local right = rect_obj.pos.x + rect_obj.w / 2
        local top = rect_obj.pos.y - rect_obj.h / 2
        local bottom = rect_obj.pos.y + rect_obj.h / 2

        if self.vec.x == 0 then
            if self.pos.x < left or self.pos.x > right then
                return false
            end
            t_near_x = -32767
            t_far_x = 32767
        else
            t_near_x = (left - self.pos.x) / self.vec.x
            t_far_x = (right - self.pos.x) / self.vec.x
            if t_near_x > t_far_x then t_near_x, t_far_x = t_far_x, t_near_x end
        end

        if self.vec.y == 0 then
            if self.pos.y < top or self.pos.y > bottom then
                return false
            end
            t_near_y = -32767
            t_far_y = 32767
        else
            t_near_y = (top - self.pos.y) / self.vec.y
            t_far_y = (bottom - self.pos.y) / self.vec.y
            if t_near_y > t_far_y then t_near_y, t_far_y = t_far_y, t_near_y end
        end

        local t_hit_near = max(t_near_x, t_near_y)
        local t_hit_far = min(t_far_x, t_far_y)

        if t_hit_far < t_hit_near or t_hit_far < 0 then
            return false
        end

        return t_hit_near
    end,

    -- Ray to tile collision using DDA (Digital Differential Analyzer) algorithm
    rayToTiles = function(self, map_id, tile_size)
        if not map_id or not self.world or not self.world.collision then
            return false
        end

        tile_size = tile_size or 16
        local collision_system = self.world.collision

        -- Starting position
        local start_x = self.pos.x
        local start_y = self.pos.y

        -- Normalized direction vector
        local dir_x = self.vec.x
        local dir_y = self.vec.y
        local length = sqrt(dir_x * dir_x + dir_y * dir_y)
        if length == 0 then return false end

        dir_x = dir_x / length
        dir_y = dir_y / length

        -- Current position along the ray
        local current_x = start_x
        local current_y = start_y

        -- Step size for ray marching
        local step_size = tile_size / 4 -- Quarter-tile precision for accuracy

        -- March along the ray
        local distance = 0
        while distance < self.length do
            -- Get current tile
            local tile_x = flr(current_x / tile_size)
            local tile_y = flr(current_y / tile_size)
            local tile_id = mget(tile_x, tile_y)

            -- Check if this tile is solid
            if collision_system:isSolidTile(tile_id) then
                -- Found a solid tile, calculate precise intersection
                local tile_left = tile_x * tile_size
                local tile_right = (tile_x + 1) * tile_size
                local tile_top = tile_y * tile_size
                local tile_bottom = (tile_y + 1) * tile_size

                -- Calculate intersection with tile bounds
                local t_min = -1
                local intersections = {}

                -- Left edge
                if dir_x ~= 0 then
                    local t = (tile_left - start_x) / dir_x
                    local y = start_y + t * dir_y
                    if t >= 0 and y >= tile_top and y <= tile_bottom then
                        add(intersections, t)
                    end
                end

                -- Right edge
                if dir_x ~= 0 then
                    local t = (tile_right - start_x) / dir_x
                    local y = start_y + t * dir_y
                    if t >= 0 and y >= tile_top and y <= tile_bottom then
                        add(intersections, t)
                    end
                end

                -- Top edge
                if dir_y ~= 0 then
                    local t = (tile_top - start_y) / dir_y
                    local x = start_x + t * dir_x
                    if t >= 0 and x >= tile_left and x <= tile_right then
                        add(intersections, t)
                    end
                end

                -- Bottom edge
                if dir_y ~= 0 then
                    local t = (tile_bottom - start_y) / dir_y
                    local x = start_x + t * dir_x
                    if t >= 0 and x >= tile_left and x <= tile_right then
                        add(intersections, t)
                    end
                end

                -- Find the closest valid intersection
                local min_t = self.length
                for _, t in pairs(intersections) do
                    if t >= 0 and t < min_t then
                        min_t = t
                    end
                end

                if min_t < self.length then
                    return min_t
                end
            end

            -- Move to next step
            current_x = current_x + dir_x * step_size
            current_y = current_y + dir_y * step_size
            distance = distance + step_size
        end

        return false
    end,

    cast = function(self)
        if self.timer:hasElapsed(self.rate) then
            local closest_hit_obj = nil
            local min_t = self.length

            -- Check world bounds collision
            local world_t = self:rayToWorld({ x = 0, y = 0, w = self.world.w, h = self.world.h })
            if world_t ~= false and world_t >= 0 and world_t < min_t then
                min_t = world_t
                closest_hit_obj = "world"
            end

            -- Check tile collision
            if self.world.map_id then
                local tile_t = self:rayToTiles(self.world.map_id, self.world.tile_size or 16)
                if tile_t ~= false and tile_t >= 0 and tile_t < min_t then
                    min_t = tile_t
                    closest_hit_obj = "tile"
                end
            end

            -- Check entity collision
            for _, obj in pairs(self.world.entities) do
                local t = false
                if obj:is_a(Circle) then
                    t = self:rayToCircle(obj)
                elseif obj:is_a(Rectangle) then
                    t = self:rayToRect(obj)
                end

                if t ~= false and t >= 0 and t < min_t then
                    min_t = t
                    closest_hit_obj = obj
                end
            end

            if closest_hit_obj ~= nil then
                self.line_dist = min_t
                self.cached_result = { closest_hit_obj, min_t }
            else
                self.line_dist = self.length
                self.cached_result = { nil, self.length }
            end
        end

        return self.cached_result[1], self.cached_result[2]
    end,
    draw_debug = function(self)
        local world_end_x = self.pos.x + self.vec.x * self.line_dist
        local world_end_y = self.pos.y + self.vec.y * self.line_dist

        self.world.gfx:line(self.pos.x, self.pos.y, world_end_x, world_end_y, 8)
    end
})

return Ray
