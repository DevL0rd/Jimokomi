local Entity = include("src/primitives/Entity.lua")
local Vector = include("src/classes/Vector.lua")

local Rectangle = Entity:new({
    _type = "Rectangle",
    w = 16,
    h = 16,
    stroke = function(self)
        if self.stroke_color > -1 then
            local x = self.pos.x - self.w / 2
            local y = self.pos.y - self.h / 2
            self.layer.gfx:rect(x, y, x + self.w - 1, y + self.h - 1, self.stroke_color)
        end
    end,
    fill = function(self)
        if self.fill_color > -1 then
            local x = self.pos.x - self.w / 2
            local y = self.pos.y - self.h / 2
            self.layer.gfx:rectfill(x, y, x + self.w - 1, y + self.h - 1, self.fill_color)
        end
    end,
    getTopLeft = function(self)
        return Vector:new({
            x = self.pos.x - self.w / 2,
            y = self.pos.y - self.h / 2
        })
    end,
    draw_debug = function(self)
        local x = self.pos.x - self.w / 2
        local y = self.pos.y - self.h / 2
        self.layer.gfx:rect(x, y, x + self.w - 1, y + self.h - 1, 8)
    end
})
return Rectangle
