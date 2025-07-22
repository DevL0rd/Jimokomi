local Timer = include("src/classes/Timer.lua")
local Particle = include("src/primitives/Particle.lua")
local Rectangle = include("src/primitives/Rectangle.lua")

local ParticleEmitter = Rectangle:new({
    _type = "ParticleEmitter",
    rate = 100,
    rate_variation = 0,
    particle_lifetime = 500,
    particle_lifetime_variation = 0,
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
            if self.rate_variation > 0 then
                self.waiting_time = self.rate + random_int(-self.rate_variation, self.rate_variation)
            else
                self.waiting_time = self.rate
            end
            local particle_lifetime = self.particle_lifetime
            if self.particle_lifetime_variation > 0 then
                particle_lifetime = self.particle_lifetime +
                    random_int(-self.particle_lifetime_variation, self.particle_lifetime_variation)
            end
            local p = self.Particle:new({
                world = self.world,
                lifetime = particle_lifetime,
            })
            local min_x = self.pos.x - self.w / 2
            local max_x = self.pos.x + self.w / 2
            local min_y = self.pos.y - self.h / 2
            local max_y = self.pos.y + self.h / 2
            p.pos:randomize(min_x, max_x, min_y, max_y)
        end
        Rectangle.update(self)
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
