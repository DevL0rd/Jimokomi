-- Marble Texture Implementation
MarbleTexture = Class:new({
    _type = "MarbleTexture"
})

-- Override the marble method in ProceduralTextures
ProceduralTextures.marble = function(self, x, y, w, h, seed, scale, use_cache)
    scale = scale or 0.03

    local render_function = function(self, sx, sy, w, h, seed, scale)
        for i = 0, w - 1 do
            for j = 0, h - 1 do
                local marble_val = self.noise:warpedNoise2d((sx + i) * scale, (sy + j) * scale, 8, seed)
                marble_val = sin(marble_val * 10) * 0.5 + 0.5 -- Create marble veins

                local color
                if marble_val > 0.8 then
                    color = 7  -- White marble
                elseif marble_val > 0.6 then
                    color = 6  -- Light gray
                elseif marble_val > 0.3 then
                    color = 5  -- Medium gray
                else
                    color = 13 -- Dark veins
                end

                pset(sx + i, sy + j, color)
            end
        end
    end

    self:renderTexture("marble", x, y, w, h, seed, use_cache, render_function, scale)
end
