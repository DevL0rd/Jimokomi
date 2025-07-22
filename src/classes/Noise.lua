local Class = include("src/classes/class.lua")

local Noise = Class:new({
    _type = "Noise",
    current_seed = 12345,

    -- Seed-based random number generator for deterministic noise
    setSeed = function(self, seed)
        self.current_seed = seed
        srand(seed)
    end,

    -- ===== HASH AND BASIC NOISE =====

    -- Hash function for consistent pseudo-random values
    hash = function(self, x, y, seed)
        seed = seed or self.current_seed
        local hash_val = ((x * 73856093) ~ (y * 19349663) ~ (seed * 83492791)) % 2147483647
        return (hash_val % 1000) / 1000 -- Return 0-1
    end,

    -- Simple 2D noise function
    noise2d = function(self, x, y, seed)
        seed = seed or self.current_seed
        return self:hash(flr(x), flr(y), seed)
    end,

    -- ===== SMOOTH NOISE =====

    -- Utility functions
    lerp = function(self, a, b, t)
        return a + t * (b - a)
    end,

    smoothstep = function(self, t)
        return t * t * (3 - 2 * t)
    end,

    -- Smooth noise with interpolation
    smoothNoise2d = function(self, x, y, seed)
        seed = seed or self.current_seed

        local intX = flr(x)
        local intY = flr(y)
        local fracX = x - intX
        local fracY = y - intY

        -- Get corner values
        local a = self:noise2d(intX, intY, seed)
        local b = self:noise2d(intX + 1, intY, seed)
        local c = self:noise2d(intX, intY + 1, seed)
        local d = self:noise2d(intX + 1, intY + 1, seed)

        -- Smooth interpolation
        local i1 = self:lerp(a, b, self:smoothstep(fracX))
        local i2 = self:lerp(c, d, self:smoothstep(fracX))
        return self:lerp(i1, i2, self:smoothstep(fracY))
    end,

    -- ===== FRACTAL NOISE =====

    -- Fractal noise (multiple octaves)
    fractalNoise2d = function(self, x, y, octaves, persistence, scale, seed)
        octaves = octaves or 4
        persistence = persistence or 0.5
        scale = scale or 0.1
        seed = seed or self.current_seed

        local value = 0
        local amplitude = 1
        local frequency = scale
        local maxValue = 0

        for i = 1, octaves do
            value += self:smoothNoise2d(x * frequency, y * frequency, seed + i) * amplitude
            maxValue += amplitude
            amplitude *= persistence
            frequency *= 2
        end

        return value / maxValue
    end,

    -- ===== PERLIN-STYLE NOISE =====

    -- Simple Perlin-style noise with gradients
    perlinNoise2d = function(self, x, y, seed)
        seed = seed or self.current_seed

        local intX = flr(x)
        local intY = flr(y)
        local fracX = x - intX
        local fracY = y - intY

        -- Generate gradients for corners
        local grad_a = self:hash(intX, intY, seed) * 2 - 1
        local grad_b = self:hash(intX + 1, intY, seed) * 2 - 1
        local grad_c = self:hash(intX, intY + 1, seed) * 2 - 1
        local grad_d = self:hash(intX + 1, intY + 1, seed) * 2 - 1

        -- Dot products
        local dot_a = grad_a * fracX
        local dot_b = grad_b * (fracX - 1)
        local dot_c = grad_c * fracY
        local dot_d = grad_d * (fracY - 1)

        -- Interpolate
        local u = self:smoothstep(fracX)
        local v = self:smoothstep(fracY)

        local i1 = self:lerp(dot_a, dot_b, u)
        local i2 = self:lerp(dot_c, dot_d, u)
        local value = self:lerp(i1, i2, v)

        -- Normalize to 0-1
        return (value + 1) / 2
    end,

    -- ===== SIMPLEX-STYLE NOISE =====

    -- Simplified simplex-style noise
    simplexNoise2d = function(self, x, y, seed)
        seed = seed or self.current_seed

        -- Skew coordinates
        local s = (x + y) * 0.366025403 -- (sqrt(3)-1)/2
        local i = flr(x + s)
        local j = flr(y + s)

        local t = (i + j) * 0.211324865 -- (3-sqrt(3))/6
        local x0 = x - (i - t)
        local y0 = y - (j - t)

        -- Determine which triangle we're in
        local i1, j1
        if x0 > y0 then
            i1, j1 = 1, 0
        else
            i1, j1 = 0, 1
        end

        local x1 = x0 - i1 + 0.211324865
        local y1 = y0 - j1 + 0.211324865
        local x2 = x0 - 1.0 + 2.0 * 0.211324865
        local y2 = y0 - 1.0 + 2.0 * 0.211324865

        -- Calculate contributions from each corner
        local n0, n1, n2 = 0, 0, 0

        local t0 = 0.5 - x0 * x0 - y0 * y0
        if t0 >= 0 then
            local grad0 = self:hash(i, j, seed) * 2 - 1
            n0 = t0 * t0 * t0 * t0 * grad0
        end

        local t1 = 0.5 - x1 * x1 - y1 * y1
        if t1 >= 0 then
            local grad1 = self:hash(i + i1, j + j1, seed) * 2 - 1
            n1 = t1 * t1 * t1 * t1 * grad1
        end

        local t2 = 0.5 - x2 * x2 - y2 * y2
        if t2 >= 0 then
            local grad2 = self:hash(i + 1, j + 1, seed) * 2 - 1
            n2 = t2 * t2 * t2 * t2 * grad2
        end

        local value = 70.0 * (n0 + n1 + n2)
        -- Normalize to 0-1
        return (value + 1) / 2
    end,

    -- ===== RIDGED NOISE =====

    -- Ridged multifractal noise
    ridgedNoise2d = function(self, x, y, octaves, gain, offset, seed)
        octaves = octaves or 4
        gain = gain or 0.5
        offset = offset or 1.0
        seed = seed or self.current_seed

        local result = 0
        local frequency = 1
        local amplitude = 0.5

        for i = 1, octaves do
            local n = self:smoothNoise2d(x * frequency, y * frequency, seed + i)
            n = offset - abs(n)
            n = n * n
            result += n * amplitude
            frequency *= 2
            amplitude *= gain
        end

        return result
    end,

    -- ===== TURBULENCE =====

    -- Turbulence (absolute value of fractal noise)
    turbulence2d = function(self, x, y, octaves, persistence, scale, seed)
        octaves = octaves or 4
        persistence = persistence or 0.5
        scale = scale or 0.1
        seed = seed or self.current_seed

        local value = 0
        local amplitude = 1
        local frequency = scale

        for i = 1, octaves do
            local n = self:smoothNoise2d(x * frequency, y * frequency, seed + i)
            value += abs(n - 0.5) * amplitude
            amplitude *= persistence
            frequency *= 2
        end

        return value
    end,

    -- ===== WARPED NOISE =====

    -- Domain-warped noise
    warpedNoise2d = function(self, x, y, warp_strength, seed)
        warp_strength = warp_strength or 4.0
        seed = seed or self.current_seed

        -- Generate warp offsets
        local warp_x = self:fractalNoise2d(x * 0.1, y * 0.1, 3, 0.5, 1, seed) * warp_strength
        local warp_y = self:fractalNoise2d(x * 0.1, y * 0.1, 3, 0.5, 1, seed + 1000) * warp_strength

        -- Sample noise at warped coordinates
        return self:fractalNoise2d(x + warp_x, y + warp_y, 4, 0.5, 0.05, seed + 2000)
    end,

    -- ===== CELLULAR AUTOMATA =====

    -- Cellular noise (like Worley/Voronoi)
    cellularNoise2d = function(self, x, y, cell_size, seed)
        cell_size = cell_size or 8
        seed = seed or self.current_seed

        local cell_x = flr(x / cell_size)
        local cell_y = flr(y / cell_size)

        local min_dist = 999999

        -- Check 3x3 grid of cells
        for dx = -1, 1 do
            for dy = -1, 1 do
                local check_x = cell_x + dx
                local check_y = cell_y + dy

                -- Get point position within cell
                local point_x = check_x * cell_size + self:hash(check_x, check_y, seed) * cell_size
                local point_y = check_y * cell_size + self:hash(check_x, check_y, seed + 1) * cell_size

                -- Calculate distance
                local dist_x = x - point_x
                local dist_y = y - point_y
                local dist = sqrt(dist_x * dist_x + dist_y * dist_y)

                min_dist = min(min_dist, dist)
            end
        end

        local value = min_dist / cell_size
        return min(1, value) -- Clamp to 0-1
    end
})

return Noise
