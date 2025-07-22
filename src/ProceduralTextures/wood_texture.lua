-- Wood Texture Implementation
WoodTexture = Class:new({
    _type = "WoodTexture"
})

-- Override the wood method in ProceduralTextures
ProceduralTextures.wood = function(self, x, y, w, h, seed, grain_scale, use_cache)
    grain_scale = grain_scale or 0.1

    local render_function = function(self, sx, sy, w, h, seed, grain_scale)
        for i = 0, w - 1 do
            for j = 0, h - 1 do
                -- Wood grain pattern
                local grain = sin((sx + i) * grain_scale +
                    self.noise:fractalNoise2d((sx + i) * 0.02, (sy + j) * 0.02, 2, 0.5, 1, seed) * 10)
                local noise_val = (grain + 1) / 2 -- Normalize to 0-1

                local color
                if noise_val > 0.6 then
                    color = 4  -- Light wood
                else
                    color = 15 -- Dark wood
                end

                pset(sx + i, sy + j, color)
            end
        end
    end

    self:renderTexture("wood", x, y, w, h, seed, use_cache, render_function, grain_scale)
end
