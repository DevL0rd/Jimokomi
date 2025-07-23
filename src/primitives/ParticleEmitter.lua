local Timer = include("src/classes/Timer.lua")
local Particle = include("src/primitives/Particle.lua")
local Rectangle = include("src/primitives/Rectangle.lua")
local Vector = include("src/classes/Vector.lua")

local ParticleEmitter = Rectangle:new({
    _type = "ParticleEmitter",
    rate = 100,
    rate_variation = 0,
    particle_lifetime = 500,
    particle_lifetime_variation = 0,
    particle_size = 3,
    particle_size_variation = 2,
    particle_accel = Vector:new({ x = 0, y = 0 }),
    vec = Vector:new({ x = 0, y = 0 }),
    vec_variation = 0,
    ignore_physics = true,
    state = true,
    Particle = Particle,
    init = function(self)
        Rectangle.init(self)
        self.spawn_timer = Timer:new()
        self.waiting_time = self.rate
    end,
    update = function(self)
        if self.state and self.spawn_timer:hasElapsed(self.waiting_time) then
            self.waiting_time = self.rate + random_int(-self.rate_variation, self.rate_variation)
            local p = self.Particle:new({
                layer = self.layer,
                lifetime = self.particle_lifetime +
                    random_int(-self.particle_lifetime_variation, self.particle_lifetime_variation),
                pos = Vector:new({
                    x = self.pos.x + random_int(-self.w / 2, self.w / 2),
                    y = self.pos.y + random_int(-self.h / 2, self.h / 2)
                }),
                vel = Vector:new({
                    x = self.vec.x + random_int(-self.vec_variation, self.vec_variation),
                    y = self.vec.y + random_int(-self.vec_variation, self.vec_variation)
                }),
                r = self.particle_size + random_int(-self.particle_size_variation, self.particle_size_variation),
                accel = self.particle_accel,
            })
        end
        Rectangle.update(self)
    end,
    draw_debug = function(self)
        local x = self.pos.x - self.w / 2
        local y = self.pos.y - self.h / 2
        self.layer.gfx:rect(x, y, x + self.w - 1, y + self.h - 1, 16)
        -- Draw the vec
        local vec_end_x = self.pos.x + self.vec.x * 0.25
        local vec_end_y = self.pos.y + self.vec.y * 0.25
        self.layer.gfx:line(self.pos.x, self.pos.y, vec_end_x, vec_end_y, 16)
    end,
    toggle = function(self)
        self.state = not self.state
    end,
    on = function(self)
        self.state = true
    end,
    off = function(self)
        self.state = false
    end
})

return ParticleEmitter
