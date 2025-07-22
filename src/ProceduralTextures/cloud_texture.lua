-- Cloud Texture Implementation
CloudTexture = Class:new({
    _type = "CloudTexture"
})

-- Override the clouds method in ProceduralTextures
ProceduralTextures.clouds = function(self, x, y, w, h, seed, density, use_cache)
    density = density or 0.02

    local render_function = function(self, sx, sy, w, h, seed, density)
        for i = 0, w - 1 do
            for j = 0, h - 1 do
                local noise_val = self.noise:fractalNoise2d((sx + i) * density, (sy + j) * density, 4, 0.5, 1, seed)
                local color = 7 -- White for clouds

                if noise_val > 0.6 then
                    pset(sx + i, sy + j, color)
                elseif noise_val > 0.4 then
                    pset(sx + i, sy + j, 6) -- Light gray
                end
            end
        end
    end

    self:renderTexture("clouds", x, y, w, h, seed, use_cache, render_function, density)
end
