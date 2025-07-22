-- Grass Texture Implementation (Animated - No Caching)
GrassTexture = Class:new({
    _type = "GrassTexture"
})

-- Override the grass method in ProceduralTextures
ProceduralTextures.grass = function(self, x, y, w, h, seed, wind_strength, use_cache)
    wind_strength = wind_strength or 0.5
    -- Note: Grass is animated and not suitable for caching

    local screen_pos = self.camera:worldToScreen({ x = x, y = y })
    local sx, sy = screen_pos.x, screen_pos.y
    local current_time = time()

    for i = 0, w - 1 do
        for j = 0, h - 1 do
            local base_noise = self.noise:fractalNoise2d((sx + i) * 0.1, (sy + j) * 0.1, 3, 0.5, 1, seed)
            local wind_noise = self.noise:smoothNoise2d((sx + i) * 0.02 + current_time * wind_strength,
                (sy + j) * 0.02, seed + 200)
            local grass_val = (base_noise + wind_noise * 0.3)

            local color
            if grass_val > 0.7 then
                color = 11 -- Light green
            elseif grass_val > 0.3 then
                color = 3  -- Medium green
            else
                color = 19 -- Dark green
            end

            pset(sx + i, sy + j, color)
        end
    end
end
