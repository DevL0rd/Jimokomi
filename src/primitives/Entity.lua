--[[pod_format="raw",created="2025-07-23 05:38:50",modified="2025-07-23 05:40:25",revision=1]]
local Class = include("src/classes/class.lua")
local Vector = include("src/classes/Vector.lua")
local Timer = include("src/classes/Timer.lua")

local Entity = Class:new({
    _type = "Entity",
    name = "Entity",
    pos = Vector:new({
        x = 0,
        y = 0
    }),
    vel = Vector:new(),
    accel = Vector:new(),
    friction = 10,
    stroke_color = -1,
    fill_color = -1,
    debug = false,
    lifetime = -1,
    ignore_physics = false,
    ignore_gravity = false,
    ignore_friction = false,
    ignore_collisions = false,
    parent = nil,
    world = nil,
    init = function(self)
        self.init_pos = Vector:new({
            x = self.pos.x,
            y = self.pos.y
        })
        self.timer = Timer:new()
        self.percent_expired = 0
        if self.parent then
            self.world = self.parent.world
        end
        if self.world then
            self.world:add(self)
        end
    end,
    update = function(self)
        if self.lifetime ~= -1 then
            self.percent_expired = self.timer:elapsed() / self.lifetime
            if self.timer:hasElapsed(self.lifetime) then
                self:destroy()
            end
        end
    end,
    draw = function(self)
        if self.fill then
            self:fill()
        end
        if self.stroke then
            self:stroke()
        end
        if self.debug then
            self:draw_debug()
            local vx = self.vel.x * 0.25
            local vy = self.vel.y * 0.25
            self.world.gfx:line(self.pos.x, self.pos.y, self.pos.x + vx, self.pos.y + vy, 8)
        end
    end,
    draw_debug = function(self)
    end,
    on_collision = function(self, ent, vector)
        if self.debug then
            local collision_scale = 10
            local end_x = self.pos.x + (vector.x * collision_scale)
            local end_y = self.pos.y + (vector.y * collision_scale)

            self.world.gfx:line(self.pos.x, self.pos.y, end_x, end_y, 8)

            self.world.gfx:circfill(end_x, end_y, 2, 8)
        end
        if self.parent then
            self.parent:on_collision(ent, vector)
        end
    end,
    destroy = function(self)
        self._delete = true
    end,
})

return Entity
