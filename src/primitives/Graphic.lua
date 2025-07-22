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
})

return Graphic
