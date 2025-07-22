--[[pod_format="raw",created="2025-07-22 06:42:11",modified="2025-07-22 06:51:35",revision=11]]
-- Water Texture Implementation (Animated - No Caching)
WaterTexture = Class:new({
    _type = "WaterTexture"
})

-- Override the water method in ProceduralTextures
ProceduralTextures.water = function(self, x, y, w, h, seed, offset, use_cache)
    offset = offset or 0

    local render_function = function(self, sx, sy, w, h, seed, offset)
        offset = offset or 0
        offset = offset * 0.01
        for i = 0, w - 1 do
            for j = 0, h - 1 do
                local wave1 = self.noise:smoothNoise2d((sx + i) * 0.02 + offset * 0.5, (sy + j) * 0.02, seed)
                local wave2 = self.noise:smoothNoise2d((sx + i) * 0.05, (sy + j) * 0.05 + offset * 0.3, seed + 100)
                local water_val = (wave1 + wave2) / 2

                local color
                if water_val > 0.7 then
                    color = 12 -- Light blue
                elseif water_val > 0.4 then
                    color = 1  -- Medium blue
                else
                    color = 5  -- Dark blue
                end

                pset(sx + i, sy + j, color)
            end
        end
    end

    self:renderTexture("water", x, y, w, h, seed, use_cache, render_function, offset)
end
