#include "InputRoutingSystem.h"

#include "../Scene.h"

void InputRoutingSystem_Update(struct Scene* scene, const struct SceneInputState* input_state, float dt_seconds)
{
    if (scene == NULL || scene->on_input == NULL)
    {
        return;
    }

    scene->on_input(scene, input_state, dt_seconds, scene->user_data);
}
