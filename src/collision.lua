Collision = Class:new({
    _type = "Collision",
    layer = nil,
    collectCollisions = function(self)
        if not self.layer.physics_enabled then return end

        for index, ent in pairs(self.layer.entities) do
            if ent.ignore_physics or ent.ignore_collisions then
                goto skip
            end
            ent.collisions = {}

            local world_coll = nil
            if ent:is_a(Rectangle) then
                world_coll = self:rectToWorld(ent)
            elseif ent:is_a(Circle) then
                world_coll = self:circleToWorld(ent)
            end

            if world_coll then
                add(ent.collisions, {
                    object = "world",
                    vector = world_coll
                })
            end

            local map_coll = self:checkMapCollision(ent)
            if map_coll then
                add(ent.collisions, {
                    object = "map",
                    vector = map_coll
                })
            end

            ::skip::
        end
    end,

    processCollisions = function(self)
        if not self.layer.physics_enabled then return end

        for index, ent in pairs(self.layer.entities) do
            if ent.ignore_physics or ent.ignore_collisions or not ent.collisions then
                goto skip
            end

            local total_push = Vector:new({ x = 0, y = 0 })
            local has_x_collision = false
            local has_y_collision = false

            for _, collision in pairs(ent.collisions) do
                total_push:add(collision.vector)
                if collision.vector.x ~= 0 then
                    has_x_collision = true
                end
                if collision.vector.y ~= 0 then
                    has_y_collision = true
                end
            end

            if total_push.x ~= 0 or total_push.y ~= 0 then
                ent.pos:add(total_push)
                ent.vel:drag(self.layer.wall_friction, false)

                if has_x_collision then
                    ent.vel.x = 0
                end
                if has_y_collision then
                    ent.vel.y = 0
                end
            end

            ::skip::
        end
    end,

    resolveCollisions = function(self)
        if not self.layer.physics_enabled then return end

        for i = 1, self.layer.collisionPasses do
            self:collectCollisions()
            self:processCollisions()
        end
    end,

    handleCollisionEvents = function(self)
        if not self.layer.physics_enabled then return end

        for index, ent in pairs(self.layer.entities) do
            if ent.collisions and #ent.collisions > 0 and ent.on_collision then
                for _, collision in pairs(ent.collisions) do
                    ent:on_collision(collision.object, collision.vector)
                end
            end
        end
    end,

    circleToWorld = function(self, cir)
        local push_x = 0
        local push_y = 0

        if cir.pos.x - cir.r < 0 then
            push_x = cir.r - cir.pos.x
        elseif cir.pos.x + cir.r > self.layer.w then
            push_x = self.layer.w - (cir.pos.x + cir.r)
        end

        if cir.pos.y - cir.r < 0 then
            push_y = cir.r - cir.pos.y
        elseif cir.pos.y + cir.r > self.layer.h then
            push_y = self.layer.h - (cir.pos.y + cir.r)
        end

        if push_x ~= 0 or push_y ~= 0 then
            return Vector:new({ x = push_x, y = push_y })
        else
            return false
        end
    end,

    rectToWorld = function(self, rec)
        local push_x = 0
        local push_y = 0

        local left = rec.pos.x - rec.w / 2
        local right = rec.pos.x + rec.w / 2
        local top = rec.pos.y - rec.h / 2
        local bottom = rec.pos.y + rec.w / 2

        if left < 0 then
            push_x = -left
        elseif right > self.layer.w then
            push_x = self.layer.w - right
        end

        if top < 0 then
            push_y = -top
        elseif bottom > self.layer.h then
            push_y = self.layer.h - bottom
        end

        if push_x ~= 0 or push_y ~= 0 then
            return Vector:new({ x = push_x, y = push_y })
        else
            return false
        end
    end,

    checkMapCollision = function(self, ent)
        if self.layer.map_id == nil then return false end

        local push_x = 0
        local push_y = 0

        if ent:is_a(Rectangle) then
            local left = ent.pos.x - ent.w / 2
            local right = ent.pos.x + ent.w / 2
            local top = ent.pos.y - ent.h / 2
            local bottom = ent.pos.y + ent.h / 2

            local tl = self.layer:getTileAt(left, top)
            local tr = self.layer:getTileAt(right, top)
            local bl = self.layer:getTileAt(left, bottom)
            local br = self.layer:getTileAt(right, bottom)

            if self.layer:isSolidTile(tl) or self.layer:isSolidTile(tr) or self.layer:isSolidTile(bl) or self.layer:isSolidTile(br) then
                local tile_x = flr(ent.pos.x / self.layer.tile_size) * self.layer.tile_size
                local tile_y = flr(ent.pos.y / self.layer.tile_size) * self.layer.tile_size

                local overlap_left = (tile_x + self.layer.tile_size) - left
                local overlap_right = right - tile_x
                local overlap_top = (tile_y + self.layer.tile_size) - top
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
        elseif ent:is_a(Circle) then
            local tile = self.layer:getTileAt(ent.pos.x, ent.pos.y)
            if self.layer:isSolidTile(tile) then
                local tile_x = flr(ent.pos.x / self.layer.tile_size) * self.layer.tile_size
                local tile_y = flr(ent.pos.y / self.layer.tile_size) * self.layer.tile_size
                local tile_center_x = tile_x + self.layer.tile_size / 2
                local tile_center_y = tile_y + self.layer.tile_size / 2

                local dx = ent.pos.x - tile_center_x
                local dy = ent.pos.y - tile_center_y
                local dist = sqrt(dx * dx + dy * dy)

                if dist < ent.r + self.layer.tile_size / 2 then
                    local push_dist = (ent.r + self.layer.tile_size / 2) - dist
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
    end
})
