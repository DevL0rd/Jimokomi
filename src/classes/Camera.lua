local Class = include("src/classes/Class.lua")
local Vector = include("src/classes/Vector.lua")
local Screen = include("src/classes/Screen.lua")


local Camera = Class:new({
    _type = "Camera",
    pos = Vector:new({ x = 0, y = 0 }),
    target = nil,
    offset = Vector:new({ x = Screen.w / 2, y = Screen.h / 2 }),
    smoothing = 0.1,
    bounds = {
        min_x = nil,
        max_x = nil,
        min_y = nil,
        max_y = nil
    },
    shake = {
        intensity = 0,
        duration = 0,
        timer = 0
    },
    parallax_factor = Vector:new({ x = 1, y = 1 }),
    update = function(self)
        if self.target then
            local target_pos = self.target:getCenter()
            local desired_x = target_pos.x - self.offset.x
            local desired_y = target_pos.y - self.offset.y

            self.pos.x += (desired_x - self.pos.x) * self.smoothing
            self.pos.y += (desired_y - self.pos.y) * self.smoothing
        end

        if self.bounds.min_x then
            self.pos.x = max(self.pos.x, self.bounds.min_x)
        end
        if self.bounds.max_x then
            self.pos.x = min(self.pos.x, self.bounds.max_x)
        end
        if self.bounds.min_y then
            self.pos.y = max(self.pos.y, self.bounds.min_y)
        end
        if self.bounds.max_y then
            self.pos.y = min(self.pos.y, self.bounds.max_y)
        end

        if self.shake.timer > 0 then
            self.shake.timer -= _dt
            if self.shake.timer <= 0 then
                self.shake.intensity = 0
            end
        end
    end,

    getShakeOffset = function(self)
        if self.shake.intensity <= 0 then
            return { x = 0, y = 0 }
        end
        return {
            x = (rnd(2) - 1) * self.shake.intensity,
            y = (rnd(2) - 1) * self.shake.intensity
        }
    end,

    startShake = function(self, intensity, duration)
        self.shake.intensity = intensity
        self.shake.duration = duration
        self.shake.timer = duration
    end,

    setTarget = function(self, entity)
        self.target = entity
    end,

    setBounds = function(self, min_x, max_x, min_y, max_y)
        self.bounds.min_x = min_x
        self.bounds.max_x = max_x
        self.bounds.min_y = min_y
        self.bounds.max_y = max_y
    end,

    worldToScreen = function(self, world_pos)
        local shake = self:getShakeOffset()
        return {
            x = world_pos.x - (self.pos.x * self.parallax_factor.x) + shake.x,
            y = world_pos.y - (self.pos.y * self.parallax_factor.y) + shake.y
        }
    end,

    screenToWorld = function(self, screen_pos)
        local shake = self:getShakeOffset()
        return {
            x = screen_pos.x + (self.pos.x * self.parallax_factor.x) - shake.x,
            y = screen_pos.y + (self.pos.y * self.parallax_factor.y) - shake.y
        }
    end,

    linkToCamera = function(self, master_camera, parallax_x, parallax_y)
        parallax_x = parallax_x or 1
        parallax_y = parallax_y or 1
        self.parallax_factor.x = parallax_x
        self.parallax_factor.y = parallax_y
        self.pos.x = master_camera.pos.x * parallax_x
        self.pos.y = master_camera.pos.y * parallax_y
    end
})

return Camera
