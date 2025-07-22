--[[pod_format="raw",created="2025-07-18 11:32:17",modified="2025-07-21 21:24:13",revision=177]]
local function deepcopy(original, visited)
    visited = visited or {}

    -- Check if we've already copied this object
    if visited[original] then
        return visited[original]
    end

    local copy = {}
    -- Store the copy in visited before recursing to handle circular references
    visited[original] = copy

    for key, value in pairs(original) do
        if type(value) == "table" then
            copy[key] = deepcopy(value, visited)
            local mt = getmetatable(value)
            if mt then
                setmetatable(copy[key], mt)
            end
        else
            copy[key] = value
        end
    end
    return copy
end

Class = {
    _type = "Class",
    _has_init = false,
    new = function(self, tbl)
        local instance = deepcopy(self)

        setmetatable(instance, { __index = self })

        tbl = tbl or {}
        for k, v in pairs(tbl) do
            instance[k] = v
        end


        if instance._type == nil then
            instance._type = self._type
        end
        if self._type ~= "Class" and self._type ~= instance._type then
            instance._type = self._type .. ":" .. instance._type
        end
        instance:init()

        return instance
    end,
    is_a = function(self, obj)
        if not obj or type(obj) ~= "table" or obj._type == nil then
            return false
        end
        return sub(self._type, 1, #obj._type) == obj._type
    end,
    init = function(self)
    end,
    unInit = function(self)
    end
}
