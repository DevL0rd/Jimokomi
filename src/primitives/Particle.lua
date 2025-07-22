local Circle = include("src/primitives/circle.lua")
local Vector = include("src/classes/vector.lua")

local Particle = Circle:new({
    _type = "Particle",
    lifetime = 3000,
    fill_color = 7,
    ignore_gravity = true,
    init = function(self)
        Circle.init(self)
        self.constant_vel = Vector:new()
        self.constant_vel:randomize(-5, 5, -50, -10)
        self.r = random_int(2, 5)
        self.initial_radius = self.r
    end,
    update = function(self)
        local d = 1 - self.percent_expired
        self.r = self.initial_radius * d
        self.vel = self.constant_vel
        DEBUG_TEXT = "Particle Update:\n" ..
            "Position: " .. tostring(self.pos) .. "\n" ..
            "Velocity: " .. tostring(self.vel) .. "\n" ..
            "Radius: " .. tostring(self.r) .. "\n"
        Circle.update(self)
    end
})

return Particle
