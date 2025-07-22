-- Stone Texture Implementation
StoneTexture = Class:new({
    _type = "StoneTexture"
})

-- Override the stone method in ProceduralTextures
ProceduralTextures.stone = function(self, x, y, w, h, seed, scale, use_cache)
    scale = scale or 0.05

    local render_function = function(self, sx, sy, w, h, seed, scale)
        for i = 0, w - 1 do
            for j = 0, h - 1 do
                local noise_val = self.noise:fractalNoise2d((sx + i) * scale, (sy + j) * scale, 3, 0.6, 1, seed)
                local color

                if noise_val > 0.7 then
                    color = 6  -- Light stone
                elseif noise_val > 0.4 then
                    color = 5  -- Medium stone
                else
                    color = 13 -- Dark stone
                end

                pset(sx + i, sy + j, color)
            end
        end
    end

    self:renderTexture("stone", x, y, w, h, seed, use_cache, render_function, scale)
end
