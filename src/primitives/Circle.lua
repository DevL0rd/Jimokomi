local Entity = include("src/classes/Entity.lua")

local Circle = Entity:new({
    _type = "Circle",
    r = 8,
    stroke = function(self)
        if self.stroke_color > -1 then
            self.world.gfx:circ(self.pos.x, self.pos.y, self.r, self.stroke_color)
        end
    end,
    fill = function(self)
        if self.fill_color > -1 then
            self.world.gfx:circfill(self.pos.x, self.pos.y, self.r, self.fill_color)
        end
    end,
    draw_debug = function(self)
        self.world.gfx:circ(self.pos.x, self.pos.y, self.r, 8)
    end
})

return Circle
