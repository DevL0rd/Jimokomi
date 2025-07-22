--[[pod_format="raw",created="2025-07-22 10:00:00",modified="2025-07-22 10:00:00",revision=1]]

-- Debug script to test ParticleEmitter state behavior

-- Create a mock world for testing
local mock_world = {
    add = function(self, entity) 
        -- Do nothing for now
    end
}

-- Test the deepcopy behavior with Timer instances
function test_timer_deepcopy()
    -- Create first emitter with state = true
    local emitter1 = ParticleEmitter:new({
        state = true,
        world = mock_world
    })
    
    -- Create second emitter with state = false  
    local emitter2 = ParticleEmitter:new({
        state = false,
        world = mock_world
    })
    
    -- Check if timers are different instances
    local same_timer = emitter1.spawn_timer == emitter2.spawn_timer
    printh("Timer instances same: " .. tostring(same_timer))
    
    if emitter1.spawn_timer and emitter2.spawn_timer then
        printh("Timer1 start_time: " .. tostring(emitter1.spawn_timer.start_time))
        printh("Timer2 start_time: " .. tostring(emitter2.spawn_timer.start_time))
    end
    
    -- Test state control
    printh("=== Initial states ===")
    printh("Emitter1 state: " .. tostring(emitter1.state))
    printh("Emitter2 state: " .. tostring(emitter2.state))
    
    -- Change states
    emitter1:off()
    emitter2:on()
    
    printh("=== After off()/on() ===")
    printh("Emitter1 state after off(): " .. tostring(emitter1.state))
    printh("Emitter2 state after on(): " .. tostring(emitter2.state))
    
    return emitter1, emitter2
end

-- Run the test
test_timer_deepcopy()
