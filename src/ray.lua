Ray = Entity:new({
    pos = Vector:new(),
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

    cast = function(self)
        if self.timer:hasElapsed(self.rate) then
            local closest_hit_obj = nil
            local min_t = self.length

            local world_t = self:rayToWorld({ x = 0, y = 0, w = self.world.w, h = self.world.h })
            if world_t ~= false and world_t >= 0 and world_t < min_t then
                min_t = world_t
                closest_hit_obj = "world"
            end

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
