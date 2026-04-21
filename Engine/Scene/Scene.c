#include "SceneInternal.h"

#include "EntityInternal.h"
#include "SpatialGrid.h"
#include "Systems/CameraFollowSystem.h"
#include "Systems/RandomForceSystem.h"
#include "Systems/InputRoutingSystem.h"
#include "Systems/PhysicsSyncSystem.h"
#include "Components/ColliderComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/TransformComponent.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

#define SCENE_SPATIAL_GRID_CELL_SIZE 64.0f

static void Scene_UpdateSpatialGridBounds(Scene* scene);

static double Scene_NowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static bool Scene_ReserveEntitySlots(Scene* scene, size_t required_capacity)
{
    struct Entity** entities = NULL;
    size_t new_capacity = 0;

    if (scene == NULL)
    {
        return false;
    }

    if (scene->entity_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = scene->entity_capacity == 0 ? 8 : scene->entity_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    entities = (struct Entity**)realloc(scene->entities, sizeof(struct Entity*) * new_capacity);
    if (entities == NULL)
    {
        return false;
    }

    scene->entities = entities;
    scene->entity_capacity = new_capacity;
    return true;
}

static bool Scene_ReserveDynamicEntitySlots(Scene* scene, size_t required_capacity)
{
    struct Entity** entities = NULL;
    size_t new_capacity = 0;

    if (scene == NULL)
    {
        return false;
    }

    if (scene->dynamic_entity_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = scene->dynamic_entity_capacity == 0 ? 8 : scene->dynamic_entity_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    entities = (struct Entity**)realloc(scene->dynamic_entities, sizeof(struct Entity*) * new_capacity);
    if (entities == NULL)
    {
        return false;
    }

    scene->dynamic_entities = entities;
    scene->dynamic_entity_capacity = new_capacity;
    return true;
}

static bool Scene_ReserveEntityListSlots(
    struct Entity*** entities,
    size_t* capacity,
    size_t required_capacity
)
{
    struct Entity** resized = NULL;
    size_t new_capacity = 0U;

    if (entities == NULL || capacity == NULL)
    {
        return false;
    }

    if (*capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = *capacity == 0U ? 8U : *capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2U;
    }

    resized = (struct Entity**)realloc(*entities, sizeof(struct Entity*) * new_capacity);
    if (resized == NULL)
    {
        return false;
    }

    *entities = resized;
    *capacity = new_capacity;
    return true;
}

static bool Scene_IsSelectableEntity(const Entity* entity)
{
    return entity != NULL &&
           Entity_IsActive(entity) &&
           Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM) != NULL &&
           Entity_GetFixedComponent(entity, COMPONENT_COLLIDER) != NULL;
}

static bool Scene_IsDraggableEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity);
}

static bool Scene_IsRenderableEntity(const Entity* entity)
{
    return entity != NULL &&
           Entity_IsActive(entity) &&
           Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM) != NULL;
}

static bool Scene_IsDebugVisibleEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity);
}

static bool Scene_IsTriggerQueryEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity);
}

static bool Scene_IsProximityQueryEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity);
}

static bool Scene_AppendEntityToList(
    struct Entity*** entities,
    size_t* count,
    size_t* capacity,
    struct Entity* entity
)
{
    if (entities == NULL || count == NULL || capacity == NULL)
    {
        return false;
    }

    if (!Scene_ReserveEntityListSlots(entities, capacity, *count + 1U))
    {
        return false;
    }

    (*entities)[(*count)++] = entity;
    return true;
}

static void Scene_RemoveEntityFromList(struct Entity** entities, size_t* count, struct Entity* entity)
{
    size_t index = 0U;

    if (entities == NULL || count == NULL || entity == NULL)
    {
        return;
    }

    for (index = 0U; index < *count; ++index)
    {
        if (entities[index] == entity)
        {
            memmove(
                &entities[index],
                &entities[index + 1U],
                sizeof(struct Entity*) * (*count - index - 1U)
            );
            *count -= 1U;
            entities[*count] = NULL;
            return;
        }
    }
}

static void Scene_UpdateBoundsFromTilemap(Scene* scene)
{
    int pixel_width = 0;
    int pixel_height = 0;

    if (scene == NULL || scene->tilemap_adapter == NULL || scene->tilemap == NULL || scene->is_overlay)
    {
        scene->view.has_world_bounds = false;
        Scene_UpdateSpatialGridBounds(scene);
        return;
    }

    if (scene->tilemap_adapter->get_pixel_width == NULL || scene->tilemap_adapter->get_pixel_height == NULL)
    {
        return;
    }

    pixel_width = scene->tilemap_adapter->get_pixel_width(scene->tilemap);
    pixel_height = scene->tilemap_adapter->get_pixel_height(scene->tilemap);
    if (pixel_width > 0 && pixel_height > 0)
    {
        Scene_SetWorldBounds(scene, 0.0f, 0.0f, (float)pixel_width, (float)pixel_height);
    }
}

static void Scene_UpdateSpatialGridBounds(Scene* scene)
{
    Aabb bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool has_bounds = false;

    if (scene == NULL)
    {
        return;
    }

    if (scene->view.has_world_bounds)
    {
        bounds.min_x = scene->view.world_min_x;
        bounds.min_y = scene->view.world_min_y;
        bounds.max_x = scene->view.world_max_x;
        bounds.max_y = scene->view.world_max_y;
        has_bounds = true;
    }

    if (has_bounds)
    {
        SpatialGrid_SetBounds(&scene->spatial_grid, bounds);
    }
    else
    {
        SpatialGrid_ClearBounds(&scene->spatial_grid);
    }
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
    SpatialGrid_Init(&scene->spatial_grid, SCENE_SPATIAL_GRID_CELL_SIZE);
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

bool Scene_AddEntity(Scene* scene, struct Entity* entity)
{
    RigidBodyComponent* rigid_body = NULL;

    if (scene == NULL || entity == NULL)
    {
        return false;
    }

    if (entity->id == 0)
    {
        entity->id = scene->next_entity_id++;
    }
    else if (entity->id >= scene->next_entity_id)
    {
        scene->next_entity_id = entity->id + 1u;
    }

    if (!Scene_ReserveEntitySlots(scene, scene->entity_count + 1))
    {
        return false;
    }

    entity->scene = scene;
    Entity_MarkDirty(
        entity,
        ENTITY_DIRTY_APPEARED | ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_LAYER_SORT
    );
    scene->entities[scene->entity_count++] = entity;
    if (scene->camera_target_entity == NULL &&
        Entity_GetComponent(entity, COMPONENT_CAMERA_TARGET) != NULL)
    {
        scene->camera_target_entity = entity;
    }
    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body != NULL && rigid_body->body_type != RIGID_BODY_STATIC)
    {
        if (!Scene_ReserveDynamicEntitySlots(scene, scene->dynamic_entity_count + 1))
        {
            Scene_RemoveEntity(scene, entity);
            return false;
        }
        scene->dynamic_entities[scene->dynamic_entity_count++] = entity;
    }
    if (Scene_IsSelectableEntity(entity) &&
        !Scene_AppendEntityToList(&scene->selectable_entities, &scene->selectable_entity_count, &scene->selectable_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsDraggableEntity(entity) &&
        !Scene_AppendEntityToList(&scene->draggable_entities, &scene->draggable_entity_count, &scene->draggable_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsRenderableEntity(entity) &&
        !Scene_AppendEntityToList(&scene->renderable_entities, &scene->renderable_entity_count, &scene->renderable_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsDebugVisibleEntity(entity) &&
        !Scene_AppendEntityToList(&scene->debug_visible_entities, &scene->debug_visible_entity_count, &scene->debug_visible_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsTriggerQueryEntity(entity) &&
        !Scene_AppendEntityToList(&scene->trigger_query_entities, &scene->trigger_query_entity_count, &scene->trigger_query_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsProximityQueryEntity(entity) &&
        !Scene_AppendEntityToList(&scene->proximity_query_entities, &scene->proximity_query_entity_count, &scene->proximity_query_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Entity_GetComponent(entity, COMPONENT_RANDOM_FORCE) != NULL)
    {
        if (!Scene_AppendEntityToList(&scene->random_force_entities, &scene->random_force_entity_count, &scene->random_force_entity_capacity, entity))
        {
            Scene_RemoveEntity(scene, entity);
            return false;
        }
    }
    if (scene->physics_world != NULL)
    {
        PhysicsWorld_RegisterEntity(scene->physics_world, entity);
    }
    SpatialGrid_UpdateEntity(&scene->spatial_grid, entity);
    return true;
}

struct Entity* Scene_RemoveEntity(Scene* scene, struct Entity* entity)
{
    size_t index = 0;
    size_t dynamic_index = 0;

    if (scene == NULL || entity == NULL)
    {
        return NULL;
    }

    (void)dynamic_index;
    Scene_RemoveEntityFromList(scene->dynamic_entities, &scene->dynamic_entity_count, entity);
    Scene_RemoveEntityFromList(scene->selectable_entities, &scene->selectable_entity_count, entity);
    Scene_RemoveEntityFromList(scene->draggable_entities, &scene->draggable_entity_count, entity);
    Scene_RemoveEntityFromList(scene->renderable_entities, &scene->renderable_entity_count, entity);
    Scene_RemoveEntityFromList(scene->debug_visible_entities, &scene->debug_visible_entity_count, entity);
    Scene_RemoveEntityFromList(scene->trigger_query_entities, &scene->trigger_query_entity_count, entity);
    Scene_RemoveEntityFromList(scene->proximity_query_entities, &scene->proximity_query_entity_count, entity);
    Scene_RemoveEntityFromList(scene->random_force_entities, &scene->random_force_entity_count, entity);
    SpatialGrid_RemoveEntity(&scene->spatial_grid, entity);
    if (scene->physics_world != NULL)
    {
        PhysicsWorld_UnregisterEntity(scene->physics_world, entity);
    }

    for (index = 0; index < scene->entity_count; ++index)
    {
        if (scene->entities[index] == entity)
        {
            PhysicsWorld_RemoveBodyForEntity(scene->physics_world, entity);
            if (scene->camera_target_entity == entity)
            {
                scene->camera_target_entity = NULL;
            }
            Entity_MarkDirty(entity, ENTITY_DIRTY_REMOVED | ENTITY_DIRTY_VISIBILITY);
            memmove(&scene->entities[index],
                    &scene->entities[index + 1],
                    sizeof(struct Entity*) * (scene->entity_count - index - 1));
            scene->entity_count -= 1;
            scene->entities[scene->entity_count] = NULL;
            entity->scene = NULL;
            return entity;
        }
    }

    return NULL;
}

void Scene_SetTilemap(Scene* scene,
                        const SceneTilemapAdapter* adapter,
                        const void* tilemap,
                        const TileRule* tile_rules,
                        size_t tile_rule_count)
{
    if (scene == NULL)
    {
        return;
    }

    scene->tilemap_adapter = adapter;
    scene->tilemap = tilemap;
    scene->tile_rules = tile_rules;
    scene->tile_rule_count = tile_rule_count;
    if (scene->physics_world != NULL)
    {
        PhysicsWorld_SetTilemap(scene->physics_world, adapter, tilemap, tile_rules, tile_rule_count);
    }
    Scene_UpdateBoundsFromTilemap(scene);
}

void Scene_SetWorldBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y)
{
    if (scene == NULL)
    {
        return;
    }

    scene->view.has_world_bounds = true;
    scene->view.world_min_x = min_x;
    scene->view.world_min_y = min_y;
    scene->view.world_max_x = max_x;
    scene->view.world_max_y = max_y;
    Scene_UpdateSpatialGridBounds(scene);
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

struct Entity* Scene_PickEntityAtScreen(Scene* scene, float screen_x, float screen_y)
{
    PhysicsQueryResult hits[16];
    size_t hit_count = 0;
    size_t index = 0;
    float world_x = 0.0f;
    float world_y = 0.0f;

    if (scene == NULL || scene->is_overlay)
    {
        return NULL;
    }

    world_x = scene->view.x + screen_x;
    world_y = scene->view.y + screen_y;
    hit_count = PhysicsWorld_QueryPoint(scene->physics_world, world_x, world_y, hits, 16);
    while (hit_count > 0)
    {
        if (Entity_IsActive(hits[hit_count - 1].entity))
        {
            return hits[hit_count - 1].entity;
        }
        --hit_count;
    }

    {
        Aabb query_bounds = { world_x - 0.5f, world_y - 0.5f, world_x + 0.5f, world_y + 0.5f };
        struct Entity* query_entities[32];
        size_t query_count = Scene_QueryEntitiesInAabb(scene, query_bounds, query_entities, 32U);
        for (index = query_count; index > 0; --index)
        {
            struct Entity* entity = query_entities[index - 1];
            TransformComponent* transform = NULL;
            ColliderComponent* collider = NULL;

            if (!Entity_IsActive(entity))
            {
                continue;
            }

            transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
            collider = (ColliderComponent*)Entity_GetFixedComponent(entity, COMPONENT_COLLIDER);
            if (transform == NULL || collider == NULL)
            {
                continue;
            }

            if (collider->shape == COLLIDER_SHAPE_CIRCLE)
            {
                float dx = world_x - transform->x;
                float dy = world_y - transform->y;
                if ((dx * dx) + (dy * dy) <= collider->radius * collider->radius)
                {
                    return entity;
                }
            }
            else
            {
                float half_w = collider->width * 0.5f;
                float half_h = collider->height * 0.5f;
                if (world_x >= transform->x - half_w && world_x <= transform->x + half_w && world_y >= transform->y - half_h &&
                    world_y <= transform->y + half_h)
                {
                    return entity;
                }
            }
        }
    }

    return NULL;
}

size_t Scene_QueryEntitiesInAabb(Scene* scene, Aabb bounds, struct Entity** results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_QueryAabb(&scene->spatial_grid, bounds, results, capacity);
}

size_t Scene_QueryEntitiesInRadius(Scene* scene, Vec2 center, float radius, struct Entity** results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_QueryCircle(&scene->spatial_grid, center, radius, results, capacity);
}

size_t Scene_QueryEntitiesAlongSegment(Scene* scene, Vec2 start, Vec2 end, struct Entity** results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_QuerySegment(&scene->spatial_grid, start, end, results, capacity);
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
