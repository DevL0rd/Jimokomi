#include "Input.h"

#include <string.h>

void EngineInputBindings_set_defaults(EngineInputBindings* bindings) {
    if (bindings == 0) {
        return;
    }

    bindings->buttons[ENGINE_INPUT_ACTION_LEFT] = 0;
    bindings->buttons[ENGINE_INPUT_ACTION_RIGHT] = 1;
    bindings->buttons[ENGINE_INPUT_ACTION_UP] = 2;
    bindings->buttons[ENGINE_INPUT_ACTION_DOWN] = 3;
    bindings->buttons[ENGINE_INPUT_ACTION_JUMP] = 4;
    bindings->buttons[ENGINE_INPUT_ACTION_ACTION] = 5;
    bindings->buttons[ENGINE_INPUT_ACTION_CYCLE_TARGET] = 6;
    bindings->buttons[ENGINE_INPUT_ACTION_DEBUG_TOGGLE] = 7;
}

void EngineInput_init(EngineInput* input, const EngineInputBindings* bindings) {
    if (input == 0) {
        return;
    }

    memset(input, 0, sizeof(*input));
    if (bindings != 0) {
        input->bindings = *bindings;
    } else {
        EngineInputBindings_set_defaults(&input->bindings);
    }
}

void EngineInput_capture(EngineInput* input, const EngineInputSnapshot* snapshot) {
    int index = 0;

    if (input == 0 || snapshot == 0) {
        return;
    }

    memcpy(input->previous_state, input->state, sizeof(input->state));

    for (index = 0; index < ENGINE_INPUT_ACTION_COUNT; ++index) {
        input->state[index] = snapshot->buttons[index];
        input->pressed_state[index] = snapshot->buttons[index] && !input->previous_state[index];
        input->repeated_state[index] = snapshot->repeated[index] || input->pressed_state[index];
    }

    input->previous_mouse_buttons = input->mouse_buttons;
    input->mouse_x = snapshot->mouse_x;
    input->mouse_y = snapshot->mouse_y;
    input->mouse_buttons = snapshot->mouse_buttons;
}

bool EngineInput_is_down(const EngineInput* input, EngineInputAction action) {
    return input != 0 && action >= 0 && action < ENGINE_INPUT_ACTION_COUNT && input->state[action];
}

bool EngineInput_was_pressed(const EngineInput* input, EngineInputAction action) {
    return input != 0 && action >= 0 && action < ENGINE_INPUT_ACTION_COUNT && input->pressed_state[action];
}

bool EngineInput_was_pressed_or_repeated(const EngineInput* input, EngineInputAction action) {
    return input != 0 && action >= 0 && action < ENGINE_INPUT_ACTION_COUNT && input->repeated_state[action];
}

bool EngineInput_is_mouse_down(const EngineInput* input, uint32_t mask) {
    return input != 0 && (input->mouse_buttons & mask) != 0;
}

bool EngineInput_was_mouse_pressed(const EngineInput* input, uint32_t mask) {
    return input != 0 && (input->mouse_buttons & mask) != 0 && (input->previous_mouse_buttons & mask) == 0;
}

bool EngineInput_was_mouse_released(const EngineInput* input, uint32_t mask) {
    return input != 0 && (input->mouse_buttons & mask) == 0 && (input->previous_mouse_buttons & mask) != 0;
}
