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
#include "Components/DraggableComponent.h"
#include "Components/RandomForceComponent.h"
#include "Components/RenderableComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SelectableComponent.h"
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
    strncpy(scene->name, name != NULL ? name : "Scene", sizeof(scene->name) - 1);
    scene->active = true;
    scene->next_entity_id = 1u;
    scene->physics_world = PhysicsWorld_Create(physics_config);
    SpatialGrid_Init(&scene->spatial_grid, EngineSettings_GetDefaults()->scene_spatial_grid_cell_size);
    scene->view = (SceneView){
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

bool Scene_AddRandomForce(Scene* scene, struct Entity* entity, float force_strength, float interval_seconds)
{
    RandomForceComponent* random_force;

    if (scene == NULL || entity == NULL || Entity_GetComponent(entity, COMPONENT_RANDOM_FORCE) != NULL)
    {
        return false;
    }

    random_force = RandomForceComponent_Create();
    if (random_force == NULL)
    {
        return false;
    }

    random_force->force_strength = force_strength;
    random_force->interval_seconds = interval_seconds;
    if (!Entity_AddComponent(entity, &random_force->base))
    {
        RandomForceComponent_Destroy(random_force);
        return false;
    }
    if (!Scene_AppendEntityToList(&scene->random_force_entities, &scene->random_force_entity_count, &scene->random_force_entity_capacity, entity))
    {
        Entity_RemoveComponent(entity, COMPONENT_RANDOM_FORCE);
        RandomForceComponent_Destroy(random_force);
        return false;
    }

    return true;
}

bool Scene_GetEntityLinearVelocity(Scene* scene, struct Entity* entity, Vec2* out_velocity)
{
    return scene != NULL &&
           scene->physics_world != NULL &&
           PhysicsWorld_GetEntityLinearVelocity(scene->physics_world, entity, out_velocity);
}

bool Scene_SetEntityLinearVelocity(Scene* scene, struct Entity* entity, Vec2 velocity)
{
    return scene != NULL &&
           scene->physics_world != NULL &&
           PhysicsWorld_SetEntityLinearVelocity(scene->physics_world, entity, velocity);
}

bool Scene_ApplyEntityLinearImpulse(Scene* scene, struct Entity* entity, Vec2 impulse, bool wake)
{
    return scene != NULL &&
           scene->physics_world != NULL &&
           PhysicsWorld_ApplyEntityLinearImpulse(scene->physics_world, entity, impulse, wake);
}

bool Scene_GetEntityContactCapacity(Scene* scene, struct Entity* entity, int* out_contact_capacity)
{
    return scene != NULL &&
           scene->physics_world != NULL &&
           PhysicsWorld_GetEntityContactCapacity(scene->physics_world, entity, out_contact_capacity);
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

    return SpatialGrid_GetCellSpanForAabb(&scene->spatial_grid, bounds, out_span);
}

size_t Scene_GetSpatialGridDirtyCellSpans(const Scene* scene, SpatialGridCellSpan* results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_GetDirtyCellSpans(&scene->spatial_grid, results, capacity);
}

void Scene_Update(Scene* scene, float dt_seconds, const SceneInputState* input_state)
{
    size_t index = 0U;
    double phase_started_ms = 0.0;
    uint32_t dirty_entity_count = 0U;

    if (scene == NULL)
    {
        return;
    }

    scene->last_input_route_ms = 0.0;
    scene->last_random_force_ms = 0.0;
    scene->last_physics_sync_ms = 0.0;
    scene->last_spatial_grid_ms = 0.0;
    scene->last_camera_follow_ms = 0.0;
    scene->spatial_grid.last_incremental_update_ms = 0.0;
    scene->spatial_grid.last_incremental_dirty_entity_count = 0U;
    scene->spatial_grid.incremental_update_count = 0U;
    SpatialGrid_ClearDirtyCells(&scene->spatial_grid);

    if (scene->on_input != NULL && input_state != NULL)
    {
        phase_started_ms = Scene_NowMs();
        InputRoutingSystem_Update(scene, input_state, dt_seconds);
        scene->last_input_route_ms = Scene_NowMs() - phase_started_ms;
    }

    if (scene->random_force_entity_count > 0U)
    {
        phase_started_ms = Scene_NowMs();
        RandomForceSystem_Update(scene, dt_seconds);
        scene->last_random_force_ms = Scene_NowMs() - phase_started_ms;
    }

    phase_started_ms = Scene_NowMs();
    PhysicsSyncSystem_Update(scene, dt_seconds);
    scene->last_physics_sync_ms = Scene_NowMs() - phase_started_ms;

    phase_started_ms = Scene_NowMs();
    for (index = 0U; index < scene->renderable_entity_count; ++index)
    {
        Entity* entity = scene->renderable_entities[index];
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
            SpatialGrid_UpdateEntity(&scene->spatial_grid, entity);
            dirty_entity_count += 1U;
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
    scene->last_spatial_grid_ms = Scene_NowMs() - phase_started_ms;
    scene->last_spatial_grid_dirty_entity_count = dirty_entity_count;
    scene->last_spatial_grid_dirty_cell_count = scene->spatial_grid.last_dirty_cell_count;
    scene->spatial_grid.last_incremental_update_ms = scene->last_spatial_grid_ms;
    scene->spatial_grid.last_incremental_dirty_entity_count = dirty_entity_count;
    scene->spatial_grid.incremental_update_count = dirty_entity_count;

    if (scene->camera_target_entity != NULL)
    {
        phase_started_ms = Scene_NowMs();
        CameraFollowSystem_Update(scene, dt_seconds);
        scene->last_camera_follow_ms = Scene_NowMs() - phase_started_ms;
    }
}

void Scene_Destroy(Scene* scene)
{
    size_t index = 0;

    if (scene == NULL)
    {
        return;
    }

    for (index = scene->entity_count; index > 0; --index)
    {
        struct Entity* entity = scene->entities[index - 1];
        if (entity != NULL)
        {
            PhysicsWorld_RemoveBodyForEntity(scene->physics_world, entity);
            entity->scene = NULL;
            Entity_Destroy(entity);
        }
    }

    free(scene->entities);
    scene->entities = NULL;
    scene->entity_count = 0;
    scene->entity_capacity = 0;
    free(scene->dynamic_entities);
    scene->dynamic_entities = NULL;
    scene->dynamic_entity_count = 0;
    scene->dynamic_entity_capacity = 0;
    free(scene->selectable_entities);
    scene->selectable_entities = NULL;
    scene->selectable_entity_count = 0;
    scene->selectable_entity_capacity = 0;
    free(scene->draggable_entities);
    scene->draggable_entities = NULL;
    scene->draggable_entity_count = 0;
    scene->draggable_entity_capacity = 0;
    free(scene->renderable_entities);
    scene->renderable_entities = NULL;
    scene->renderable_entity_count = 0;
    scene->renderable_entity_capacity = 0;
    free(scene->debug_visible_entities);
    scene->debug_visible_entities = NULL;
    scene->debug_visible_entity_count = 0;
    scene->debug_visible_entity_capacity = 0;
    free(scene->trigger_query_entities);
    scene->trigger_query_entities = NULL;
    scene->trigger_query_entity_count = 0;
    scene->trigger_query_entity_capacity = 0;
    free(scene->proximity_query_entities);
    scene->proximity_query_entities = NULL;
    scene->proximity_query_entity_count = 0;
    scene->proximity_query_entity_capacity = 0;
    free(scene->random_force_entities);
    scene->random_force_entities = NULL;
    scene->random_force_entity_count = 0;
    scene->random_force_entity_capacity = 0;

    if (scene->physics_world != NULL)
    {
        PhysicsWorld_Destroy(scene->physics_world);
        scene->physics_world = NULL;
    }

    SpatialGrid_Dispose(&scene->spatial_grid);

    free(scene);
}
