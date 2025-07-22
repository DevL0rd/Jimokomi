local Class = include("src/classes/Class.lua")
local Noise = include("src/classes/Noise.lua")

-- Base Procedural Textures System
local ProceduralTextures = Class:new({
    _type = "ProceduralTextures",
    camera = nil,
    noise = nil,
    texture_cache = {},  -- Cache for rendered textures
    cache_keys = {},     -- Track cache keys for LRU eviction
    max_cache_size = 20, -- Maximum number of cached textures
    init = function(self)
        self.noise = Noise:new()
        self.texture_cache = {}
        self.cache_keys = {}
    end,

    -- ===== CACHE MANAGEMENT =====

    -- Clear texture cache (call when memory gets tight)
    clearTextureCache = function(self)
        self.texture_cache = {}
        self.cache_keys = {}
    end,

    -- Remove oldest cache entry when limit is reached
    evictOldestCacheEntry = function(self)
        if #self.cache_keys > 0 then
            local oldest_key = self.cache_keys[1]
            self.texture_cache[oldest_key] = nil
            -- Remove from cache_keys array
            for i = 1, #self.cache_keys - 1 do
                self.cache_keys[i] = self.cache_keys[i + 1]
            end
            self.cache_keys[#self.cache_keys] = nil
        end
    end,

    -- Generate cache key for texture parameters
    generateTextureKey = function(self, texture_type, w, h, seed, ...)
        local extra_params = { ... }
        local key = texture_type .. "_" .. w .. "_" .. h .. "_" .. seed
        for i = 1, #extra_params do
            key = key .. "_" .. extra_params[i]
        end
        return key
    end,

    -- Check if texture is cached and render from cache if available
    getCachedTexture = function(self, cache_key)
        return self.texture_cache[cache_key]
    end,

    -- Store rendered texture data in cache
    setCachedTexture = function(self, cache_key, texture_data)
        -- Check if we need to evict old entries
        while #self.cache_keys >= self.max_cache_size do
            self:evictOldestCacheEntry()
        end

        -- Add new texture to cache
        self.texture_cache[cache_key] = texture_data
        self.cache_keys[#self.cache_keys + 1] = cache_key
    end,

    -- Render cached texture data to screen
    renderCachedTexture = function(self, texture_data, sx, sy)
        for i = 1, #texture_data do
            local pixel = texture_data[i]
            pset(sx + pixel.x, sy + pixel.y, pixel.color)
        end
    end,

    -- Capture texture data for caching
    captureTextureData = function(self, sx, sy, w, h)
        local texture_data = {}
        for i = 0, w - 1 do
            for j = 0, h - 1 do
                local color = pget(sx + i, sy + j)
                if color and color ~= 0 then -- Only store non-transparent pixels
                    texture_data[#texture_data + 1] = { x = i, y = j, color = color }
                end
            end
        end
        return texture_data
    end,

    -- ===== BASE TEXTURE RENDERING SYSTEM =====

    -- Generic render method with caching support
    renderTexture = function(self, texture_type, x, y, w, h, seed, use_cache, render_function, ...)
        seed = seed or self.noise.current_seed
        use_cache = use_cache or false

        local screen_pos = self.camera:worldToScreen({ x = x, y = y })
        local sx, sy = screen_pos.x, screen_pos.y

        -- Check cache if enabled
        if use_cache then
            local cache_key = self:generateTextureKey(texture_type, w, h, seed, ...)
            local cached_texture = self:getCachedTexture(cache_key)
            if cached_texture then
                self:renderCachedTexture(cached_texture, sx, sy)
                return
            end
        end

        -- Generate texture using provided render function
        render_function(self, sx, sy, w, h, seed, ...)

        -- Cache the result if enabled
        if use_cache then
            local cache_key = self:generateTextureKey(texture_type, w, h, seed, ...)
            local texture_data = self:captureTextureData(sx, sy, w, h)
            self:setCachedTexture(cache_key, texture_data)
        end
    end,
})

return ProceduralTextures
