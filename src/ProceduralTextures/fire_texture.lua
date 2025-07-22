-- Fire Texture Implementation (Animated - No Caching)
FireTexture = Class:new({
    _type = "FireTexture"
})

-- Override the fire method in ProceduralTextures
ProceduralTextures.fire = function(self, x, y, w, h, seed, time_offset, use_cache)
    time_offset = time_offset or 0

    local render_function = function(self, sx, sy, w, h, seed, time_offset)
        local current_time = time() + time_offset

        for i = 0, w - 1 do
            for j = 0, h - 1 do
                local height_factor = 1 - (j / h) -- Fire is brighter at bottom
                local turbulence = self.noise:fractalNoise2d((sx + i) * 0.05, (sy + j) * 0.05 + current_time, 3, 0.5, 1,
                    seed)
                local fire_val = (turbulence + height_factor) / 2

                local color
                if fire_val > 0.8 then
                    color = 7  -- White hot
                elseif fire_val > 0.6 then
                    color = 10 -- Yellow
                elseif fire_val > 0.3 then
                    color = 9  -- Orange
                elseif fire_val > 0.1 then
                    color = 8  -- Red
                end

                if color then
                    pset(sx + i, sy + j, color)
                end
            end
        end
    end

    self:renderTexture("fire", x, y, w, h, seed, use_cache, render_function, time_offset)
end
