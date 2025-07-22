-- SaveSystem: Easy-to-use save/load system for serializable classes
local Class = include("src/classes/Class.lua")

local SaveSystem = {
    registry = {},

    -- Register a class so it can be properly deserialized
    register_class = function(self, class_obj)
        if class_obj._type then
            self.registry[class_obj._type] = class_obj
        end
    end,

    -- Register multiple classes at once
    register_classes = function(self, classes)
        for _, class_obj in pairs(classes) do
            self:register_class(class_obj)
        end
    end,

    -- Save a single object to a file
    save_object = function(self, obj, filename)
        if not obj or not obj.serialize then
            return false, "Object is not serializable"
        end

        local serialized = obj:serialize()
        if not serialized then
            return false, "Serialization failed"
        end

        local save_data = {
            version = "1.0",
            timestamp = time(),
            object = serialized
        }

        -- Use Picotron's store function
        store(filename, save_data)
        return true, "Saved successfully"
    end,

    -- Load a single object from a file
    load_object = function(self, filename)
        local save_data = fetch(filename)
        if not save_data then
            return nil, "File not found or couldn't be loaded"
        end

        if not save_data.object then
            return nil, "No object data found in save file"
        end

        local loaded = Class.deserialize(save_data.object, self.registry)
        if not loaded then
            return nil, "Deserialization failed"
        end

        return loaded, "Loaded successfully"
    end,

    -- Save multiple objects to a file
    save_multiple = function(self, objects, filename)
        local save_data = {
            version = "1.0",
            timestamp = time(),
            objects = {}
        }

        local errors = {}

        for name, obj in pairs(objects) do
            if obj and obj.serialize then
                local serialized = obj:serialize()
                if serialized then
                    save_data.objects[name] = serialized
                else
                    add(errors, "Failed to serialize object: " .. name)
                end
            else
                add(errors, "Object not serializable: " .. name)
            end
        end

        if #errors > 0 then
            local error_msg = "Errors occurred: "
            for i, err in pairs(errors) do
                error_msg = error_msg .. err
                if i < #errors then
                    error_msg = error_msg .. ", "
                end
            end
            return false, error_msg
        end

        store(filename, save_data)
        return true, "Saved " .. #save_data.objects .. " objects successfully"
    end,

    -- Load multiple objects from a file
    load_multiple = function(self, filename)
        local save_data = fetch(filename)
        if not save_data then
            return nil, "File not found or couldn't be loaded"
        end

        if not save_data.objects then
            return nil, "No objects data found in save file"
        end

        local loaded = {}
        local errors = {}

        for name, data in pairs(save_data.objects) do
            local obj = Class.deserialize(data, self.registry)
            if obj then
                loaded[name] = obj
            else
                add(errors, "Failed to deserialize: " .. name)
            end
        end

        if #errors > 0 then
            local error_msg = "Loaded with errors: "
            for i, err in pairs(errors) do
                error_msg = error_msg .. err
                if i < #errors then
                    error_msg = error_msg .. ", "
                end
            end
            return loaded, error_msg
        end

        return loaded, "Loaded " .. #loaded .. " objects successfully"
    end,

    -- Save game state (wrapper for save_multiple with common game objects)
    save_game_state = function(self, player, level_data, settings, filename)
        local game_state = {
            player = player,
            level = level_data,
            settings = settings
        }

        return self:save_multiple(game_state, filename or "savegame.dat")
    end,

    -- Load game state
    load_game_state = function(self, filename)
        return self:load_multiple(filename or "savegame.dat")
    end,

    -- Quick save for single object (uses object type as filename)
    quick_save = function(self, obj, prefix)
        if not obj or not obj._type then
            return false, "Object has no type for filename"
        end

        local filename = (prefix or "quick_") .. obj._type .. ".dat"
        return self:save_object(obj, filename)
    end,

    -- Quick load for single object type
    quick_load = function(self, object_type, prefix)
        local filename = (prefix or "quick_") .. object_type .. ".dat"
        return self:load_object(filename)
    end,

    -- Check if a save file exists
    save_exists = function(self, filename)
        local data = fetch(filename)
        return data ~= nil
    end,

    -- Get save file info without loading it
    get_save_info = function(self, filename)
        local save_data = fetch(filename)
        if not save_data then
            return nil, "File not found"
        end

        local info = {
            version = save_data.version or "unknown",
            timestamp = save_data.timestamp or 0,
            has_single_object = save_data.object ~= nil,
            object_count = 0
        }

        if save_data.objects then
            for _ in pairs(save_data.objects) do
                info.object_count = info.object_count + 1
            end
        end

        return info
    end,

    -- Create a backup of a save file
    backup_save = function(self, filename, backup_suffix)
        local data = fetch(filename)
        if not data then
            return false, "Original file not found"
        end

        local backup_name = filename .. (backup_suffix or ".backup")
        store(backup_name, data)
        return true, "Backup created: " .. backup_name
    end,

    -- Validate that an object can be safely serialized
    validate_serializable = function(self, obj)
        if not obj then
            return false, "Object is nil"
        end

        if not obj.serialize then
            return false, "Object does not have serialize method"
        end

        local serialized = obj:serialize()
        if not serialized then
            return false, "Serialization returned nil"
        end

        -- Check for issues
        local info = obj:get_serialization_info()
        local warnings = {}

        if #info.errors > 0 then
            add(warnings, #info.errors .. " serialization errors")
        end

        if #info.skipped_functions > 0 then
            add(warnings, #info.skipped_functions .. " functions skipped")
        end

        local result_msg = "Object is fully serializable"
        if #warnings > 0 then
            result_msg = ""
            for i, warning in pairs(warnings) do
                result_msg = result_msg .. warning
                if i < #warnings then
                    result_msg = result_msg .. ", "
                end
            end
        end

        return true, result_msg
    end
}

return SaveSystem
