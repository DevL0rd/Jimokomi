local Vector = include("src/classes/Vector.lua")

local Screen = {
    w = 480,
    h = 270
}

function Screen:getCenter()
    return Vector:new({
        x = self.w / 2,
        y = self.h / 2
    })
end

function Screen:getAspectRatio()
    return self.w / self.h
end

return Screen
