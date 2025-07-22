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
    end,

    -- Gradient drawing functions
    drawLinearGradient = function(self, x1, y1, x2, y2, color1, color2)
        local sx1, sy1 = self:worldToScreen(x1, y1)
        local sx2, sy2 = self:worldToScreen(x2, y2)
        local dx = sx2 - sx1
        local dy = sy2 - sy1
        local distance = sqrt(dx * dx + dy * dy)

        for x = sx1, sx2 do
            for y = sy1, sy2 do
                local pixel_dx = x - sx1
                local pixel_dy = y - sy1
                local pixel_dist = sqrt(pixel_dx * pixel_dx + pixel_dy * pixel_dy)
                local t = pixel_dist / distance
                t = max(0, min(1, t)) -- Clamp between 0 and 1

                local color = flr(color1 + (color2 - color1) * t)
                pset(x, y, color)
            end
        end
    end,

    drawRadialGradient = function(self, cx, cy, inner_radius, outer_radius, inner_color, outer_color)
        local scx, scy = self:worldToScreen(cx, cy)
        for x = scx - outer_radius, scx + outer_radius do
            for y = scy - outer_radius, scy + outer_radius do
                local dx = x - scx
                local dy = y - scy
                local distance = sqrt(dx * dx + dy * dy)

                if distance <= outer_radius then
                    local t = (distance - inner_radius) / (outer_radius - inner_radius)
                    t = max(0, min(1, t))

                    local color = flr(inner_color + (outer_color - inner_color) * t)
                    pset(x, y, color)
                end
            end
        end
    end,

    drawHorizontalGradient = function(self, x, y, w, h, left_color, right_color)
        local sx, sy = self:worldToScreen(x, y)
        for i = 0, w - 1 do
            local t = i / (w - 1)
            local color = flr(left_color + (right_color - left_color) * t)
            line(sx + i, sy, sx + i, sy + h - 1, color)
        end
    end,

    drawVerticalGradient = function(self, x, y, w, h, top_color, bottom_color)
        local sx, sy = self:worldToScreen(x, y)
        for i = 0, h - 1 do
            local t = i / (h - 1)
            local color = flr(top_color + (bottom_color - top_color) * t)
            line(sx, sy + i, sx + w - 1, sy + i, color)
        end
    end,

    -- Advanced gradient and pattern functions
    drawDitheredGradient = function(self, x, y, w, h, color1, color2, pattern)
        -- Use fillp() for dithered patterns between colors
        local old_fillp = peek(0x5f34) -- Save current fillp

        -- Apply dither pattern (0x0000 to 0xFFFF)
        pattern = pattern or 0xAA55 -- Default checkerboard pattern
        fillp(pattern)

        local sx, sy = self:worldToScreen(x, y)
        rectfill(sx, sy, sx + w - 1, sy + h - 1, color1)

        fillp(band(bnot(pattern), 0xFFFF)) -- Invert pattern
        rectfill(sx, sy, sx + w - 1, sy + h - 1, color2)

        fillp(old_fillp) -- Restore fillp
    end,

    drawPaletteGradient = function(self, x, y, w, h, start_pal, end_pal, steps)
        -- Use pal() to create smooth color transitions
        steps = steps or 16
        local sx, sy = self:worldToScreen(x, y)

        -- Save current palette
        local old_pal = {}
        for i = 0, 15 do
            old_pal[i] = peek(0x3000 + i)
        end

        local step_height = h / steps

        for i = 0, steps - 1 do
            local t = i / (steps - 1)
            local interp_color = flr(start_pal + (end_pal - start_pal) * t)

            -- Remap a specific color to our interpolated color
            pal(7, interp_color) -- Remap white to gradient color

            local rect_y = sy + i * step_height
            rectfill(sx, rect_y, sx + w - 1, rect_y + step_height, 7)
        end

        -- Restore palette
        for i = 0, 15 do
            poke(0x3000 + i, old_pal[i])
        end
        pal() -- Reset palette
    end,

    drawAnimatedGradient = function(self, x, y, w, h, colors, speed)
        -- Animated gradient using time-based color cycling
        speed = speed or 1
        local time_offset = flr(time() * speed * 100) % #colors

        local sx, sy = self:worldToScreen(x, y)

        for i = 0, h - 1 do
            local color_index = ((i + time_offset) % #colors) + 1
            line(sx, sy + i, sx + w - 1, sy + i, colors[color_index])
        end
    end,

    drawWaveGradient = function(self, x, y, w, h, color1, color2, frequency, amplitude)
        -- Sine wave-based gradient
        frequency = frequency or 0.1
        amplitude = amplitude or 0.5

        local sx, sy = self:worldToScreen(x, y)

        for i = 0, w - 1 do
            local wave = sin(i * frequency + time()) * amplitude + 0.5
            local color = flr(color1 + (color2 - color1) * wave)
            line(sx + i, sy, sx + i, sy + h - 1, color)
        end
    end,

    setDitherPattern = function(self, pattern)
        -- Helper to set custom dither patterns
        -- Common patterns:
        -- 0x0000 = solid
        -- 0xAA55 = checkerboard
        -- 0x5A5A = diagonal lines
        -- 0x7777 = dotted
        fillp(pattern)
    end,

    resetFillPattern = function(self)
        fillp(0x0000) -- Reset to solid fill
    end
})
