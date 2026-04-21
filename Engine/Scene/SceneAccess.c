#include "SceneInternal.h"

#include "SpatialGrid.h"
#include "../Physics/PhysicsWorld.h"

#include <string.h>

void Scene_SetPhysicsPaused(Scene* scene, bool paused)
{
    if (scene != NULL)
    {
        scene->physics_paused = paused;
    }
}

bool Scene_IsPhysicsPaused(const Scene* scene)
{
    return scene != NULL && scene->physics_paused;
}

void Scene_SetUserData(Scene* scene, void* user_data)
{
    if (scene != NULL)
    {
        scene->user_data = user_data;
    }
}

void* Scene_GetUserData(const Scene* scene)
{
    return scene != NULL ? scene->user_data : NULL;
}

void Scene_SetInputCallback(Scene* scene, SceneInputCallback callback)
{
    if (scene != NULL)
    {
        scene->on_input = callback;
    }
}

PhysicsWorld* Scene_GetPhysicsWorld(Scene* scene)
{
    return scene != NULL ? scene->physics_world : NULL;
}

const PhysicsWorld* Scene_GetPhysicsWorldConst(const Scene* scene)
{
    return scene != NULL ? scene->physics_world : NULL;
}

SceneView Scene_GetViewSnapshot(const Scene* scene)
{
    if (scene == NULL)
    {
        return (SceneView){ 0 };
    }

    return scene->view;
}

void Scene_SetView(Scene* scene, const SceneView* view)
{
    if (scene == NULL || view == NULL)
    {
        return;
    }

    scene->view = *view;
}

size_t Scene_GetEntityCount(const Scene* scene)
{
    return scene != NULL ? scene->entity_count : 0U;
}

void Scene_GetStatsSnapshot(const Scene* scene, SceneStatsSnapshot* out_snapshot)
{
    if (out_snapshot == NULL)
    {
        return;
    }
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    if (scene == NULL)
    {
        return;
    }

    out_snapshot->entity_count = scene->entity_count;
    out_snapshot->input_route_ms = scene->last_input_route_ms;
    out_snapshot->random_force_ms = scene->last_random_force_ms;
    out_snapshot->physics_sync_ms = scene->last_physics_sync_ms;
    out_snapshot->camera_follow_ms = scene->last_camera_follow_ms;
    out_snapshot->physics_substeps = scene->last_physics_substeps;
}

void Scene_GetSpatialGridStatsSnapshot(const Scene* scene, SpatialGridStatsSnapshot* out_snapshot)
{
    if (out_snapshot == NULL)
    {
        return;
    }
    if (scene == NULL)
    {
        memset(out_snapshot, 0, sizeof(*out_snapshot));
        return;
    }

    SpatialGrid_GetStatsSnapshot(&scene->spatial_grid, out_snapshot);
}
