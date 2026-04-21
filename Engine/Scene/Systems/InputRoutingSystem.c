#include "InputRoutingSystem.h"

#include "../SceneInternal.h"

static bool input_routing_is_idle(const struct SceneInputState* input_state)
{
    size_t index = 0U;

    if (input_state == NULL)
    {
        return true;
    }

    for (index = 0U; index < sizeof(input_state->buttons) / sizeof(input_state->buttons[0]); ++index)
    {
        if (input_state->buttons[index])
        {
            return false;
        }
    }

    return !input_state->mouse_primary_down &&
           !input_state->mouse_primary_pressed &&
           !input_state->mouse_primary_released;
}

void InputRoutingSystem_Update(struct Scene* scene, const struct SceneInputState* input_state, float dt_seconds)
{
    if (scene == NULL || scene->lifecycle.on_input == NULL || input_routing_is_idle(input_state))
    {
        return;
    }

    scene->lifecycle.on_input(scene, input_state, dt_seconds, scene->lifecycle.user_data);
}
