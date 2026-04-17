local WorldObject = include("src/primitives/WorldObject.lua")

local Graphic = WorldObject:new({
    _type = "Graphic",
    shape = {
        kind = "rect",
        w = 16,
        h = 16,
    },
    flip_x = false,
    flip_y = false,
    ignore_physics = true,
    init = function(self)
        WorldObject.init(self)
    end,
    draw_debug = function(self)
        self:strokeShape(32)
    end,
})

return Graphic
