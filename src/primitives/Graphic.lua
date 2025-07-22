local Rectangle = include("src/primitives/Rectangle.lua")
local Vector = include("src/classes/Vector.lua")

local Graphic = Rectangle:new({
    _type = "Graphic",
    flip_x = false,
    flip_y = false,
    ignore_physics = true,
    init = function(self)
        Rectangle.init(self)
        self.pos = Vector:new()
    end,
    draw_debug = function(self)
        local x = self.pos.x - self.w / 2
        local y = self.pos.y - self.h / 2
        self.world.gfx:rect(x, y, x + self.w - 1, y + self.h - 1, 32)
    end,
})

return Graphic
