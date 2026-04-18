local WorldObject = include("src/Engine/Objects/WorldObject.lua")

local Particle = WorldObject:new({
    _type = "Particle",
    entity_kind = 1,
    inherit_layer_debug = false,
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
        local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
        if profiler then
            profiler:addCounter("render.particle.updated", 1)
        end
        local d = 1 - self.percent_expired
        self:setCircleShape(self.initial_radius * d)
        WorldObject.update(self)
    end,
    draw = function(self)
        local profiler = self.layer and self.layer.engine and self.layer.engine.profiler or nil
        if profiler then
            profiler:addCounter("render.particle.draws", 1)
        end
        if self.layer and self.layer.camera and self.fill_color > -1 then
            local sx, sy = self.layer.camera:layerToScreenXY(self.pos.x, self.pos.y)
            circfill(sx, sy, max(1, flr(self.getRadius and self:getRadius() or self.shape.r or 1)), self.fill_color)
        end
        if self.debug or self.stroke or self.stroke_color > -1 or self.fill ~= nil then
            WorldObject.draw(self)
        end
    end
})

return Particle
