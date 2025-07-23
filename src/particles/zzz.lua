--[[pod_format="raw",created="2025-07-22 15:55:50",modified="2025-07-22 15:55:50",revision=0]]
local Particle = include("src/primitives/Particle.lua")
local Vector = include("src/classes/vector.lua")

local zzz = Particle:new({
    _type = "Particle",
    lifetime = 3000,
    draw = function(self)
        -- Draw the letter z floating away scaling down
        local scale = 1 - self.percent_expired
        local x = self.pos.x
        local y = self.pos.y
        if scale >= 0.5 then
            self.layer.gfx:print("Z", x, y, 7)
        elseif scale >= 0.25 then
            self.layer.gfx:print("z", x, y, 7)
        else
            self.layer.gfx:print(".", x, y, 7)
        end
    end
})

return zzz
