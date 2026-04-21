#include "SceneInternal.h"

#include "EntityInternal.h"
#include "SceneStorage.h"
#include "SceneView.h"
#include "SpatialGrid.h"
#include "Systems/CameraFollowSystem.h"
#include "Systems/RandomForceSystem.h"
#include "Systems/InputRoutingSystem.h"
#include "Systems/PhysicsSyncSystem.h"
#include "Components/ColliderComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/TransformComponent.h"
#include "../Physics/PhysicsBodyControl.h"
#include "../Settings.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

static double Scene_NowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void Scene_Init(Scene* scene, const char* name, const PhysicsWorldConfig* physics_config)
{
    if (scene == NULL)
    {
        return;
    }

    memset(scene, 0, sizeof(*scene));
    strncpy(scene->lifecycle.name, name != NULL ? name : "Scene", sizeof(scene->lifecycle.name) - 1);
    scene->lifecycle.active = true;
    scene->lifecycle.next_entity_id = 1u;
    scene->physics.physics_world = PhysicsWorld_Create(physics_config);
    SpatialGrid_Init(&scene->spatial.spatial_grid, EngineSettings_GetDefaults()->scene_spatial_grid_cell_size);
    scene->spatial.view = (SceneView){
        .previous_x = 0.0f,
        .previous_y = 0.0f,
        .previous_view_width = 480.0f,
        .previous_view_height = 270.0f,
        .x = 0.0f,
        .y = 0.0f,
        .view_width = 480.0f,
        .view_height = 270.0f,
        .smoothing = 0.12f,
        .has_world_bounds = false,
        .world_min_x = 0.0f,
        .world_min_y = 0.0f,
        .world_max_x = 0.0f,
        .world_max_y = 0.0f
    };
    Scene_UpdateSpatialGridBounds(scene);
}

Scene* Scene_Create(const char* name, const PhysicsWorldConfig* physics_config)
{
    Scene* scene = (Scene*)calloc(1, sizeof(Scene));
    if (scene == NULL)
    {
        return NULL;
    }

    Scene_Init(scene, name, physics_config);
    return scene;
}

bool Scene_GetEntityLinearVelocity(Scene* scene, struct Entity* entity, Vec2* out_velocity)
{
    return scene != NULL &&
           scene->physics.physics_world != NULL &&
           PhysicsWorld_GetEntityLinearVelocity(scene->physics.physics_world, entity, out_velocity);
}

bool Scene_SetEntityLinearVelocity(Scene* scene, struct Entity* entity, Vec2 velocity)
{
    return scene != NULL &&
           scene->physics.physics_world != NULL &&
           PhysicsWorld_SetEntityLinearVelocity(scene->physics.physics_world, entity, velocity);
}

bool Scene_ApplyEntityLinearImpulse(Scene* scene, struct Entity* entity, Vec2 impulse, bool wake)
{
    return scene != NULL &&
           scene->physics.physics_world != NULL &&
           PhysicsWorld_ApplyEntityLinearImpulse(scene->physics.physics_world, entity, impulse, wake);
}

bool Scene_GetEntityContactCapacity(Scene* scene, struct Entity* entity, int* out_contact_capacity)
{
    return scene != NULL &&
           scene->physics.physics_world != NULL &&
           PhysicsWorld_GetEntityContactCapacity(scene->physics.physics_world, entity, out_contact_capacity);
}

bool Scene_GetSpatialGridCellSpanForAabb(const Scene* scene, Aabb bounds, SpatialGridCellSpan* out_span)
{
    if (scene == NULL)
    {
        if (out_span != NULL)
        {
            memset(out_span, 0, sizeof(*out_span));
        }
        return false;
    }

    return SpatialGrid_GetCellSpanForAabb(&scene->spatial.spatial_grid, bounds, out_span);
}

size_t Scene_GetSpatialGridDirtyCellSpans(const Scene* scene, SpatialGridCellSpan* results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_GetDirtyCellSpans(&scene->spatial.spatial_grid, results, capacity);
}

void Scene_Update(Scene* scene, float dt_seconds, const SceneInputState* input_state)
{
    size_t index = 0U;
    double phase_started_ms = 0.0;

    if (scene == NULL)
    {
        return;
    }

    scene->stats.last_input_route_ms = 0.0;
    scene->stats.last_random_force_ms = 0.0;
    scene->stats.last_physics_sync_ms = 0.0;
    scene->stats.last_camera_follow_ms = 0.0;
    SpatialGrid_ClearDirtyCells(&scene->spatial.spatial_grid);

    if (scene->lifecycle.on_input != NULL && input_state != NULL)
    {
        phase_started_ms = Scene_NowMs();
        InputRoutingSystem_Update(scene, input_state, dt_seconds);
        scene->stats.last_input_route_ms = Scene_NowMs() - phase_started_ms;
    }

    if (scene->storage.random_force_entity_count > 0U)
    {
        phase_started_ms = Scene_NowMs();
        RandomForceSystem_Update(scene, dt_seconds);
        scene->stats.last_random_force_ms = Scene_NowMs() - phase_started_ms;
    }

    phase_started_ms = Scene_NowMs();
    PhysicsSyncSystem_Update(scene, dt_seconds);
    scene->stats.last_physics_sync_ms = Scene_NowMs() - phase_started_ms;

    for (index = 0U; index < scene->storage.renderable_entity_count; ++index)
    {
        Entity* entity = scene->storage.renderable_entities[index];
        TransformComponent* transform = NULL;
        ColliderComponent* collider = NULL;

        if (entity == NULL)
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
        collider = (ColliderComponent*)Entity_GetFixedComponent(entity, COMPONENT_COLLIDER);
        if (Entity_IsDirty(entity, ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_APPEARED | ENTITY_DIRTY_REMOVED | ENTITY_DIRTY_ENABLED | ENTITY_DIRTY_DISABLED) ||
            (transform != NULL && (transform->dirty_flags & (TRANSFORM_DIRTY_POSITION | TRANSFORM_DIRTY_TELEPORT)) != 0U) ||
            (collider != NULL && collider->dirty_flags != COLLIDER_DIRTY_NONE))
        {
            SpatialGrid_UpdateEntity(&scene->spatial.spatial_grid, entity);
            if (transform != NULL)
            {
                TransformComponent_ClearDirty(transform, TRANSFORM_DIRTY_POSITION | TRANSFORM_DIRTY_TELEPORT);
            }
            Entity_ClearDirty(
                entity,
                ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_APPEARED | ENTITY_DIRTY_REMOVED | ENTITY_DIRTY_ENABLED | ENTITY_DIRTY_DISABLED
            );
        }
    }

    if (scene->camera_follow.camera_target_entity != NULL)
    {
        phase_started_ms = Scene_NowMs();
        CameraFollowSystem_Update(scene, dt_seconds);
        scene->stats.last_camera_follow_ms = Scene_NowMs() - phase_started_ms;
    }
}

void Scene_Destroy(Scene* scene)
{
    size_t index = 0;

    if (scene == NULL)
    {
        return;
    }

    for (index = scene->storage.entity_count; index > 0; --index)
    {
        struct Entity* entity = scene->storage.entities[index - 1];
        if (entity != NULL)
        {
            PhysicsWorld_RemoveBodyForEntity(scene->physics.physics_world, entity);
            entity->scene = NULL;
            Entity_Destroy(entity);
        }
    }

    SceneStorage_Dispose(&scene->storage);

    if (scene->physics.physics_world != NULL)
    {
        PhysicsWorld_Destroy(scene->physics.physics_world);
        scene->physics.physics_world = NULL;
    }

    SpatialGrid_Dispose(&scene->spatial.spatial_grid);

    free(scene);
}
