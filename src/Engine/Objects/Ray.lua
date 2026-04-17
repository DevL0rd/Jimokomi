local WorldObject = include("src/Engine/Objects/WorldObject.lua")
local Vector = include("src/Engine/Math/Vector.lua")
local Timer = include("src/Engine/Core/Timer.lua")

local Ray = WorldObject:new({
    _type = "Ray",
    vec = Vector:new({ y = 1 }),
    length = 50,
    rate = 50,
    ignore_physics = true,
    ignore_collisions = true,
    snapshot_enabled = false,
    init = function(self)
        WorldObject.init(self)
        self.line_dist = self.length
        self.timer = Timer:new()
        self.cached_result = { nil, self.length }
    end,
    cast = function(self)
        if self.timer:hasElapsed(self.rate) then
            if self.layer and self.layer.world then
                local hit_obj, distance = self.layer.world:castRay(self)
                self.line_dist = distance
                self.cached_result = { hit_obj, distance }
            end
        end

        return self.cached_result[1], self.cached_result[2]
    end,
    draw_debug = function(self)
        local layer_end_x = self.pos.x + self.vec.x * self.line_dist
        local layer_end_y = self.pos.y + self.vec.y * self.line_dist

        self.layer.gfx:line(self.pos.x, self.pos.y, layer_end_x, layer_end_y, 8)
    end
})

return Ray
