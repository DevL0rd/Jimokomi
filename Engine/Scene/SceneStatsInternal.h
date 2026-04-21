#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_STATS_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_STATS_INTERNAL_H

#include "SceneInternal.h"

struct SceneStatsState {
    double last_input_route_ms;
    double last_random_force_ms;
    double last_physics_sync_ms;
    double last_camera_follow_ms;
    uint32_t last_physics_substeps;
};

#endif
