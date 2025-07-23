local Vector = include("src/classes/Vector.lua")
local Screen = include("src/classes/Screen.lua")
local Timer = include("src/classes/Timer.lua")

local Camera = {
    new = function(self, config)
        config = config or {}
        local camera = setmetatable({}, { __index = self })

        -- Position and movement
        camera.pos = config.pos or Vector:new({ x = 0, y = 0 })
        camera.target = config.target or nil
        camera.offset = config.offset or Vector:new({ x = Screen.w / 2, y = Screen.h / 2 })
        camera.smoothing = config.smoothing or 0.1

        -- Camera bounds
        camera.bounds = {
            min_x = config.min_x or nil,
            max_x = config.max_x or nil,
            min_y = config.min_y or nil,
            max_y = config.max_y or nil
        }

        -- Screen shake system
        camera.shake = {
            initial_intensity = 0,
            intensity = 0,
            duration = -1,
            timer = Timer:new()
        }

        -- Parallax support
        camera.parallax_factor = config.parallax_factor or Vector:new({ x = 1, y = 1 })

        return camera
    end,

    -- Update camera position and effects
    update = function(self)
        -- Follow target if set
        if self.target then
            local target_pos = self.target.pos
            local desired_x = target_pos.x - self.offset.x
            local desired_y = target_pos.y - self.offset.y

            self.pos.x = self.pos.x + (desired_x - self.pos.x) * self.smoothing
            self.pos.y = self.pos.y + (desired_y - self.pos.y) * self.smoothing
        end

        -- Apply bounds constraints
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

        -- Update screen shake
        if self.shake.duration ~= -1 then
            local percent_elapsed = self.shake.timer:elapsed() / self.shake.duration
            self.shake.intensity = self.shake.initial_intensity * (1 - percent_elapsed)
            if self.shake.timer:hasElapsed(self.shake.duration, false) then
                self.shake.intensity = 0
                self.shake.duration = -1
            end
        end
    end,

    -- Get current shake offset
    getShakeOffset = function(self)
        if self.shake.intensity <= 0 then
            return { x = 0, y = 0 }
        end
        return {
            x = (rnd(2) - 1) * self.shake.intensity,
            y = (rnd(2) - 1) * self.shake.intensity
        }
    end,

    -- Start screen shake effect
    startShaking = function(self, intensity, duration)
        intensity = intensity or 5
        duration = duration or -1
        self.shake.initial_intensity = intensity
        self.shake.duration = duration
        if duration ~= -1 then
            self.shake.timer:reset()
        end
    end,

    -- Stop screen shake effect
    stopShaking = function(self)
        self.shake.intensity = 0
        self.shake.duration = -1
    end,

    -- Set target entity to follow
    setTarget = function(self, entity)
        self.target = entity
    end,

    -- Set camera bounds
    setBounds = function(self, min_x, max_x, min_y, max_y)
        self.bounds.min_x = min_x
        self.bounds.max_x = max_x
        self.bounds.min_y = min_y
        self.bounds.max_y = max_y
    end,

    -- Convert layer coordinates to screen coordinates
    layerToScreen = function(self, layer_pos)
        local shake = self:getShakeOffset()
        return {
            x = layer_pos.x - (self.pos.x * self.parallax_factor.x) + shake.x,
            y = layer_pos.y - (self.pos.y * self.parallax_factor.y) + shake.y
        }
    end,

    -- Convert screen coordinates to layer coordinates
    screenToLayer = function(self, screen_pos)
        local shake = self:getShakeOffset()
        return {
            x = screen_pos.x + (self.pos.x * self.parallax_factor.x) - shake.x,
            y = screen_pos.y + (self.pos.y * self.parallax_factor.y) - shake.y
        }
    end,

    -- Link this camera to a master camera for parallax scrolling
    linkToCamera = function(self, master_camera, parallax_x, parallax_y)
        parallax_x = parallax_x or 1
        parallax_y = parallax_y or 1
        self.parallax_factor.x = parallax_x
        self.parallax_factor.y = parallax_y
        self.pos.x = master_camera.pos.x * parallax_x
        self.pos.y = master_camera.pos.y * parallax_y
    end,

    -- Set camera offset (typically screen center)
    setOffset = function(self, x, y)
        self.offset.x = x
        self.offset.y = y
    end,

    -- Set camera smoothing factor (0 = instant, 1 = no movement)
    setSmoothing = function(self, smoothing)
        self.smoothing = smoothing
    end,

    -- Set parallax factor for this camera
    setParallaxFactor = function(self, x, y)
        self.parallax_factor.x = x
        self.parallax_factor.y = y
    end,

    -- Get camera center position in layer coordinates
    getCenterPos = function(self)
        return {
            x = self.pos.x + self.offset.x,
            y = self.pos.y + self.offset.y
        }
    end,

    -- Set camera center position in layer coordinates
    setCenterPos = function(self, x, y)
        self.pos.x = x - self.offset.x
        self.pos.y = y - self.offset.y
    end,

    -- Get camera viewport bounds in layer coordinates
    getViewBounds = function(self)
        local top_left = self:screenToLayer({ x = 0, y = 0 })
        local bottom_right = self:screenToLayer({ x = Screen.w, y = Screen.h })
        return {
            left = top_left.x,
            top = top_left.y,
            right = bottom_right.x,
            bottom = bottom_right.y,
            width = bottom_right.x - top_left.x,
            height = bottom_right.y - top_left.y
        }
    end,

    -- Check if a point is visible in the camera viewport
    isPointVisible = function(self, x, y)
        local bounds = self:getViewBounds()
        return x >= bounds.left and x <= bounds.right and
            y >= bounds.top and y <= bounds.bottom
    end,

    -- Check if a rectangle is visible in the camera viewport
    isRectVisible = function(self, x, y, w, h)
        local bounds = self:getViewBounds()
        return not (x + w < bounds.left or x > bounds.right or
            y + h < bounds.top or y > bounds.bottom)
    end
}

return Camera
