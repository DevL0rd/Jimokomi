--[[pod_format="raw",created="2025-07-22 15:55:50",modified="2025-07-22 15:55:50",revision=0]]
local Circle = include("src/primitives/circle.lua")
local Vector = include("src/classes/vector.lua")

local Particle = Circle:new({
    _type = "Particle",
    lifetime = 3000,
    fill_color = 7,
    init = function(self)
        Circle.init(self)
        self.initial_radius = self.r
    end,
    update = function(self)
        local d = 1 - self.percent_expired
        self.r = self.initial_radius * d
        Circle.update(self)
    end
})

return Particle
