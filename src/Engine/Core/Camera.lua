local Vector = include("src/Engine/Math/Vector.lua")
local Screen = include("src/Engine/Core/Screen.lua")
local Timer = include("src/Engine/Core/Timer.lua")

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
            timer = Timer:new(),
            offset = { x = 0, y = 0 }
        }

        -- Parallax support
        camera.parallax_factor = config.parallax_factor or Vector:new({ x = 1, y = 1 })
        camera.view_bounds = {
            left = 0,
            top = 0,
            right = 0,
            bottom = 0,
            width = Screen.w,
            height = Screen.h
        }
        camera:updateViewBounds()

        return camera
    end,

    updateViewBounds = function(self)
        local shake = self.shake.offset
        local left = (self.pos.x * self.parallax_factor.x) - shake.x
        local top = (self.pos.y * self.parallax_factor.y) - shake.y
        self.view_bounds.left = left
        self.view_bounds.top = top
        self.view_bounds.right = left + Screen.w
        self.view_bounds.bottom = top + Screen.h
        self.view_bounds.width = Screen.w
        self.view_bounds.height = Screen.h
        return self.view_bounds
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

        if self.shake.intensity > 0 then
            self.shake.offset.x = (rnd(2) - 1) * self.shake.intensity
            self.shake.offset.y = (rnd(2) - 1) * self.shake.intensity
        else
            self.shake.offset.x = 0
            self.shake.offset.y = 0
        end

        self:updateViewBounds()
    end,

    -- Get current shake offset
    getShakeOffset = function(self)
        return self.shake.offset
    end,

    -- Start screen shake effect
    startShaking = function(self, intensity, duration)
        intensity = intensity or 5
        duration = duration or -1
        self.shake.initial_intensity = intensity
        self.shake.intensity = intensity
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
        return self.view_bounds
    end,

    -- Check if a point is visible in the camera viewport
    isPointVisible = function(self, x, y)
        local bounds = self.view_bounds
        return x >= bounds.left and x <= bounds.right and
            y >= bounds.top and y <= bounds.bottom
    end,

    -- Check if a rectangle is visible in the camera viewport
    isRectVisible = function(self, x, y, w, h)
        local bounds = self.view_bounds
        return not (x + w < bounds.left or x > bounds.right or
            y + h < bounds.top or y > bounds.bottom)
    end
}

return Camera
