--[[pod_format="raw",created="2025-07-22 15:55:50",modified="2025-07-22 15:55:50",revision=0]]
local Particle = include("src/Engine/Objects/Particle.lua")
local Vector = include("src/Engine/Math/Vector.lua")

local zzz = Particle:new({
    _type = "zzz",
    lifetime = 3000,
    draw = function(self)
        local scale = 1 - self.percent_expired
        local x = self.pos.x
        local y = self.pos.y
        local text = "."
        if scale >= 0.5 then
            text = "Z"
        elseif scale >= 0.25 then
            text = "z"
        end
        self.layer.gfx:drawCachedTextLayer(
            nil,
            text,
            x,
            y,
            7,
            {
                cache_tag = "particle.zzz",
            }
        )
    end
})

return zzz
