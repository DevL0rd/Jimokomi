#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_LIFECYCLE_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_LIFECYCLE_INTERNAL_H

#include "SceneInternal.h"

struct SceneLifecycleState {
    char name[64];
    bool is_overlay;
    bool active;
    void* user_data;
    uint32_t next_entity_id;
    void (*on_input)(struct Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data);
};

#endif
