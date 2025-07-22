-- Example: Using Class serialization system
local Class = include("src/classes/Class.lua")
local Vector = include("src/classes/Vector.lua")
local Circle = include("src/primitives/Circle.lua")
local Rectangle = include("src/primitives/Rectangle.lua")

-- Example 1: Basic serialization
function example_basic_serialization()
    -- Create a simple class
    local Player = Class:new({
        _type = "Player",
        name = "Default",
        level = 1,
        health = 100,
        position = nil,

        init = function(self)
            self.position = Vector:new({ x = 0, y = 0 })
        end,

        move = function(self, x, y)
            self.position.x = self.position.x + x
            self.position.y = self.position.y + y
        end
    })

    -- Create an instance
    local player = Player:new({
        name = "Hero",
        level = 5,
        health = 85
    })

    player:move(100, 50)

    -- Serialize the player
    local serialized = player:serialize()
    print("Serialized player data available")

    -- Check what couldn't be serialized
    local info = player:get_serialization_info()
    print("Skipped functions:", #info.skipped_functions)
    print("Errors:", #info.errors)

    return serialized, player
end

-- Example 2: Class registry for proper deserialization
function create_class_registry()
    local registry = {}

    -- Register all your classes here
    registry["Player"] = Class:new({
        _type = "Player",
        name = "Default",
        level = 1,
        health = 100,
        position = nil,

        init = function(self)
            if not self.position then
                self.position = Vector:new({ x = 0, y = 0 })
            end
        end,

        move = function(self, x, y)
            self.position.x = self.position.x + x
            self.position.y = self.position.y + y
        end,

        get_status = function(self)
            return "Level " .. self.level .. " with " .. self.health .. " health"
        end
    })

    registry["Vector"] = Vector
    registry["Circle"] = Circle
    registry["Rectangle"] = Rectangle

    return registry
end

-- Example 3: Complete save/load system
function example_save_load_system()
    local registry = create_class_registry()

    -- Create some complex objects
    local player = registry["Player"]:new({
        name = "TestHero",
        level = 10,
        health = 75
    })

    player:move(200, 100)

    local enemy = registry["Circle"]:new({
        pos = Vector:new({ x = 300, y = 200 }),
        r = 15,
        health = 50
    })

    -- Save system - serialize multiple objects
    local save_data = {
        version = "1.0",
        timestamp = time(),
        player = player:serialize(),
        enemy = enemy:serialize()
    }

    -- In a real game, you'd write this to a file
    -- store("savegame.dat", save_data)

    -- Load system - deserialize the objects
    local loaded_player = Class.deserialize(save_data.player, registry)
    local loaded_enemy = Class.deserialize(save_data.enemy, registry)

    -- Verify the loaded objects work
    if loaded_player and loaded_player.get_status then
        print("Loaded player:", loaded_player:get_status())
    end

    return {
        original = { player = player, enemy = enemy },
        loaded = { player = loaded_player, enemy = loaded_enemy },
        save_data = save_data
    }
end

-- Example 4: Handling complex nested objects
function example_complex_serialization()
    local registry = create_class_registry()

    -- Create a class with nested objects
    local GameState = Class:new({
        _type = "GameState",
        level = 1,
        score = 0,
        entities = {},

        init = function(self)
            self.entities = {}
        end,

        add_entity = function(self, entity)
            add(self.entities, entity)
        end,

        update = function(self)
            for _, entity in pairs(self.entities) do
                if entity.update then
                    entity:update()
                end
            end
        end
    })

    -- Create game state with multiple entities
    local game = GameState:new({
        level = 3,
        score = 1500
    })

    -- Add some entities
    game:add_entity(registry["Circle"]:new({
        pos = Vector:new({ x = 100, y = 100 }),
        r = 10
    }))

    game:add_entity(registry["Rectangle"]:new({
        pos = Vector:new({ x = 200, y = 150 }),
        w = 32,
        h = 24
    }))

    -- Serialize the entire game state
    local serialized = game:serialize()

    -- Deserialize back
    registry["GameState"] = GameState
    local loaded_game = Class.deserialize(serialized, registry)

    return {
        original = game,
        serialized = serialized,
        loaded = loaded_game
    }
end

-- Example 5: Simple data-only serialization
function example_simple_data_serialization()
    local Player = Class:new({
        _type = "Player",
        name = "Default",
        x = 0,
        y = 0,
        health = 100,

        move = function(self, dx, dy)
            self.x = self.x + dx
            self.y = self.y + dy
        end
    })

    local player = Player:new({
        name = "SimpleHero",
        x = 50,
        y = 25,
        health = 80
    })

    -- Use to_data for simple serialization (no functions, cleaner)
    local data = player:to_data()
    print("Simple data created")

    -- Restore from data
    local restored = Player:from_data(data)
    print("Player restored:", restored.name, restored.x, restored.y)

    return { original = player, data = data, restored = restored }
end

-- Example 6: Handling serialization errors gracefully
function example_error_handling()
    local ProblematicClass = Class:new({
        _type = "ProblematicClass",
        good_data = "serializable",
        circular_ref = nil,

        init = function(self)
            -- Create circular reference (can be handled)
            self.circular_ref = self
        end,

        problematic_function = function(self)
            return "this won't be serialized"
        end
    })

    local obj = ProblematicClass:new()

    -- Try to serialize and check for issues
    local serialized = obj:serialize()
    local info = obj:get_serialization_info()

    print("Serialization completed with:")
    print("- Functions skipped:", #info.skipped_functions)
    print("- Errors:", #info.errors)

    return { object = obj, serialized = serialized, info = info }
end

-- Example 7: Save game utility functions
function create_save_system()
    local save_system = {
        registry = create_class_registry(),

        save_object = function(self, obj, filename)
            local serialized = obj:serialize()
            if serialized then
                -- In real usage: store(filename, serialized)
                return serialized
            end
            return nil
        end,

        load_object = function(self, filename)
            -- In real usage: local data = fetch(filename)
            -- For now, return nil since we can't actually load
            return nil
        end,

        save_multiple = function(self, objects, filename)
            local save_data = {
                version = "1.0",
                objects = {}
            }

            for name, obj in pairs(objects) do
                save_data.objects[name] = obj:serialize()
            end

            return save_data
        end,

        load_multiple = function(self, save_data)
            local loaded = {}

            if save_data.objects then
                for name, data in pairs(save_data.objects) do
                    loaded[name] = Class.deserialize(data, self.registry)
                end
            end

            return loaded
        end
    }

    return save_system
end

return {
    example_basic_serialization = example_basic_serialization,
    create_class_registry = create_class_registry,
    example_save_load_system = example_save_load_system,
    example_complex_serialization = example_complex_serialization,
    example_simple_data_serialization = example_simple_data_serialization,
    example_error_handling = example_error_handling,
    create_save_system = create_save_system
}
