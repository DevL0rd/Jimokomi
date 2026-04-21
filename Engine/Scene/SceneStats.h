#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_STATS_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_STATS_H

#include <stddef.h>
#include <stdint.h>

typedef struct Scene Scene;

typedef struct SceneStatsSnapshot {
    size_t entity_count;
    double input_route_ms;
    double random_force_ms;
    double physics_sync_ms;
    double camera_follow_ms;
    uint32_t physics_substeps;
} SceneStatsSnapshot;

size_t Scene_GetEntityCount(const Scene* scene);
void Scene_GetStatsSnapshot(const Scene* scene, SceneStatsSnapshot* out_snapshot);

#endif
