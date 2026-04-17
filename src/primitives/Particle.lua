local WorldObject = include("src/primitives/WorldObject.lua")

local Particle = WorldObject:new({
    _type = "Particle",
    ignore_physics = true,
    ignore_collisions = true,
    snapshot_enabled = false,
    shape = {
        kind = "circle",
        r = 3,
    },
    lifetime = 3000,
    fill_color = 7,
    init = function(self)
        WorldObject.init(self)
        self.initial_radius = self:getRadius()
    end,
    update = function(self)
        local d = 1 - self.percent_expired
        self:setCircleShape(self.initial_radius * d)
        WorldObject.update(self)
    end
})

return Particle
