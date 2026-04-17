--[[pod_format="raw",created="2025-07-18 11:32:17",modified="2025-07-22 01:55:10",revision=178]]
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

-- Serialization helper functions
local function is_serializable_type(value)
    local t = type(value)
    return t == "string" or t == "number" or t == "boolean" or t == "table"
end

local function serialize_value(value, visited, depth)
    visited = visited or {}
    depth = depth or 0

    -- Prevent infinite recursion
    if depth > 50 then
        return nil, "max_depth_exceeded"
    end

    local t = type(value)

    if t == "string" or t == "number" or t == "boolean" then
        return value
    elseif t == "table" then
        -- Check for circular references
        if visited[value] then
            return { _circular_ref = true, _ref_id = visited[value] }
        end

        -- Assign unique ID for this table
        local ref_id = #visited + 1
        visited[value] = ref_id

        local serialized = {
            _class_data = true,
            _ref_id = ref_id
        }

        -- Store class type if it exists
        if value._type then
            serialized._type = value._type
        end

        -- Serialize all serializable fields
        for k, v in pairs(value) do
            if is_serializable_type(v) and k ~= "_type" then
                local serialized_key = serialize_value(k, visited, depth + 1)
                local serialized_val, err = serialize_value(v, visited, depth + 1)

                if serialized_val ~= nil and serialized_key ~= nil then
                    serialized[serialized_key] = serialized_val
                elseif err ~= "function_skipped" then
                    -- Only report errors that aren't just skipped functions
                    if serialized._errors == nil then
                        serialized._errors = {}
                    end
                    add(serialized._errors, "Failed to serialize key '" .. tostr(k) .. "': " .. (err or "unknown error"))
                end
            elseif type(v) == "function" then
                -- Functions are not serializable but we note them
                if serialized._skipped_functions == nil then
                    serialized._skipped_functions = {}
                end
                add(serialized._skipped_functions, tostr(k))
            end
        end

        return serialized
    else
        return nil, "function_skipped"
    end
end

local function deserialize_value(value, class_registry, object_refs)
    object_refs = object_refs or {}

    if type(value) ~= "table" then
        return value
    end

    -- Handle circular references
    if value._circular_ref then
        return object_refs[value._ref_id]
    end

    -- Handle class data
    if value._class_data then
        local deserialized = {}

        -- Store in object references for circular ref resolution
        if value._ref_id then
            object_refs[value._ref_id] = deserialized
        end

        -- Deserialize all fields
        for k, v in pairs(value) do
            if k ~= "_class_data" and k ~= "_ref_id" and k ~= "_type" and
                k ~= "_errors" and k ~= "_skipped_functions" then
                deserialized[k] = deserialize_value(v, class_registry, object_refs)
            end
        end

        -- Restore class type and methods if available
        if value._type and class_registry and class_registry[value._type] then
            setmetatable(deserialized, { __index = class_registry[value._type] })
            deserialized._type = value._type
        else
            deserialized._type = value._type
        end

        return deserialized
    else
        -- Regular table
        local deserialized = {}
        for k, v in pairs(value) do
            deserialized[k] = deserialize_value(v, class_registry, object_refs)
        end
        return deserialized
    end
end

local Class = {
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

    -- Serialize this instance to a table that can be saved
    serialize = function(self)
        local serialized, err = serialize_value(self)
        return serialized, err
    end,

    -- Static method to deserialize data back to a class instance
    deserialize = function(data, class_registry)
        return deserialize_value(data, class_registry)
    end,

    -- Get a list of what couldn't be serialized
    get_serialization_info = function(self)
        local serialized = self:serialize()
        return {
            errors = serialized._errors or {},
            skipped_functions = serialized._skipped_functions or {},
            has_circular_refs = serialized._circular_refs ~= nil
        }
    end,

    -- Create a serialization-friendly copy (removes functions and circular refs)
    to_data = function(self)
        local data = {}

        for k, v in pairs(self) do
            if is_serializable_type(v) and type(v) ~= "function" then
                if type(v) == "table" then
                    -- Recursively convert tables, but avoid circular references
                    if v.to_data and type(v.to_data) == "function" then
                        data[k] = v:to_data()
                    else
                        data[k] = v -- Simple table copy for now
                    end
                else
                    data[k] = v
                end
            end
        end

        -- Always include the type for reconstruction
        data._type = self._type

        return data
    end,

    -- Restore from data (opposite of to_data)
    from_data = function(self, data, class_registry)
        local instance = self:new()

        for k, v in pairs(data) do
            if k ~= "_type" then
                instance[k] = v
            end
        end

        return instance
    end,

    init = function(self)
    end,
    unInit = function(self)
    end
}

return Class
