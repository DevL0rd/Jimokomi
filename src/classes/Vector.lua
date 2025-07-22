local Class = include("src/classes/class.lua")

local Vector = Class:new({
    _type = "Vector",
    x = 0,
    y = 0,
    randomize = function(self, min_val_x, max_val_x, min_val_y, max_val_y)
        min_val_x = min_val_x or -1
        max_val_x = max_val_x or 1
        min_val_y = min_val_y or -1
        max_val_y = max_val_y or 1
        self.x = random_float(min_val_x, max_val_x)
        self.y = random_float(min_val_y, max_val_y)
        return self
    end,
    round = function(self)
        self.x = round(self.x)
        self.y = round(self.y)
        return self
    end,
    add = function(self, vec2, useDelta)
        useDelta = useDelta or false
        if type(vec2) == "number" then
            vec2 = {
                x = vec2,
                y = vec2
            }
        end
        if useDelta then
            self.x += vec2.x * _dt
            self.y += vec2.y * _dt
        else
            self.x += vec2.x
            self.y += vec2.y
        end
        return self
    end,
    sub = function(self, vec2, useDelta)
        useDelta = useDelta or false
        if type(vec2) == "number" then
            vec2 = {
                x = vec2,
                y = vec2
            }
        end
        if useDelta then
            self.x -= vec2.x * _dt
            self.y -= vec2.y * _dt
        else
            self.x -= vec2.x
            self.y -= vec2.y
        end
        return self
    end,
    len = function(self)
        return sqrt(self.x * self.x + self.y * self.y)
    end,
    normalize = function(self)
        local l = self:len()
        if l > 0 then
            self.x /= l
            self.y /= l
        end
        return self
    end,
    drag = function(self, dragV, useDelta)
        useDelta = useDelta or false
        local d = 1 - dragV
        if useDelta then
            local xDiff = self.x - (self.x * d)
            local yDiff = self.y - (self.y * d)
            self.x -= xDiff * _dt
            self.y -= yDiff * _dt
        else
            self.x *= d
            self.y *= d
        end
        return self
    end,
    dot = function(self, other_vec)
        return self.x * other_vec.x + self.y * other_vec.y
    end
})

return Vector
