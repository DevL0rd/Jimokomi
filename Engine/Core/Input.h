#ifndef JIMOKOMI_ENGINE_CORE_INPUT_H
#define JIMOKOMI_ENGINE_CORE_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum EngineInputAction {
    ENGINE_INPUT_ACTION_LEFT = 0,
    ENGINE_INPUT_ACTION_RIGHT,
    ENGINE_INPUT_ACTION_UP,
    ENGINE_INPUT_ACTION_DOWN,
    ENGINE_INPUT_ACTION_JUMP,
    ENGINE_INPUT_ACTION_ACTION,
    ENGINE_INPUT_ACTION_CYCLE_TARGET,
    ENGINE_INPUT_ACTION_DEBUG_TOGGLE,
    ENGINE_INPUT_ACTION_DEBUG_WORLD_TOGGLE,
    ENGINE_INPUT_ACTION_PAN_LEFT,
    ENGINE_INPUT_ACTION_PAN_RIGHT,
    ENGINE_INPUT_ACTION_PAN_UP,
    ENGINE_INPUT_ACTION_PAN_DOWN,
    ENGINE_INPUT_ACTION_ZOOM_IN,
    ENGINE_INPUT_ACTION_ZOOM_OUT,
    ENGINE_INPUT_ACTION_COUNT
} EngineInputAction;

typedef struct EngineInputBindings {
    int buttons[ENGINE_INPUT_ACTION_COUNT];
} EngineInputBindings;

typedef struct EngineInputSnapshot {
    bool buttons[ENGINE_INPUT_ACTION_COUNT];
    bool repeated[ENGINE_INPUT_ACTION_COUNT];
    int mouse_x;
    int mouse_y;
    float mouse_wheel_delta;
    uint32_t mouse_buttons;
} EngineInputSnapshot;

typedef struct EngineInput {
    EngineInputBindings bindings;
    bool state[ENGINE_INPUT_ACTION_COUNT];
    bool previous_state[ENGINE_INPUT_ACTION_COUNT];
    bool pressed_state[ENGINE_INPUT_ACTION_COUNT];
    bool repeated_state[ENGINE_INPUT_ACTION_COUNT];
    int mouse_x;
    int mouse_y;
    float mouse_wheel_delta;
    uint32_t mouse_buttons;
    uint32_t previous_mouse_buttons;
} EngineInput;

void EngineInputBindings_set_defaults(EngineInputBindings* bindings);
void EngineInput_init(EngineInput* input, const EngineInputBindings* bindings);
void EngineInput_capture(EngineInput* input, const EngineInputSnapshot* snapshot);
bool EngineInput_is_down(const EngineInput* input, EngineInputAction action);
bool EngineInput_was_pressed(const EngineInput* input, EngineInputAction action);
bool EngineInput_was_pressed_or_repeated(const EngineInput* input, EngineInputAction action);
bool EngineInput_is_mouse_down(const EngineInput* input, uint32_t mask);
bool EngineInput_was_mouse_pressed(const EngineInput* input, uint32_t mask);
bool EngineInput_was_mouse_released(const EngineInput* input, uint32_t mask);

#endif
