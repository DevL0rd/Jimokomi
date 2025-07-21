GraphicsHelper = Class:new({
    _type = "GraphicsHelper",
    camera = nil,
    worldToScreen = function(self, world_x, world_y)
        local screen_pos = self.camera:worldToScreen({ x = world_x, y = world_y })
        return screen_pos.x, screen_pos.y
    end,

    line = function(self, x1, y1, x2, y2, color)
        local sx1, sy1 = self.worldToScreen(self, x1, y1)
        local sx2, sy2 = self.worldToScreen(self, x2, y2)
        line(sx1, sy1, sx2, sy2, color)
    end,

    circ = function(self, x, y, r, color)
        local sx, sy = self:worldToScreen(x, y)
        circ(sx, sy, r, color)
    end,

    circfill = function(self, x, y, r, color)
        local sx, sy = self:worldToScreen(x, y)
        circfill(sx, sy, r, color)
    end,

    rect = function(self, x1, y1, x2, y2, color)
        local sx1, sy1 = self:worldToScreen(x1, y1)
        local sx2, sy2 = self:worldToScreen(x2, y2)
        rect(sx1, sy1, sx2, sy2, color)
    end,

    rectfill = function(self, x1, y1, x2, y2, color)
        local sx1, sy1 = self:worldToScreen(x1, y1)
        local sx2, sy2 = self:worldToScreen(x2, y2)
        rectfill(sx1, sy1, sx2, sy2, color)
    end,

    spr = function(self, sprite_id, x, y, flip_x, flip_y)
        local sx, sy = self:worldToScreen(x, y)
        spr(sprite_id, sx, sy, flip_x, flip_y)
    end,

    print = function(self, text, x, y, color)
        local sx, sy = self:worldToScreen(x, y)
        print(text, sx, sy, color)
    end
})
