#include "PhysicsWorldEntitiesInternal.h"
#include "PhysicsWorldLifecycleInternal.h"
#include "PhysicsWorldStatsInternal.h"
#include "PhysicsWorldTilemapInternal.h"

#include "PhysicsBodyControl.h"

#include "../Core/TaskSystem.h"
#include "../Scene/SceneInternal.h"
#include "../Scene/SceneStatsInternal.h"
#include "../Scene/EntityInternal.h"
#include "../Scene/Components/ColliderComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Components/TransformComponent.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void task_system_configure_box2d_world_def(const struct TaskSystem* system, b2WorldDef* world_def);

static double PhysicsWorld_NowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static bool PhysicsWorld_ReserveEntityPointerArray(struct Entity*** entities, size_t* capacity, size_t required_capacity)
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

    new_capacity = *capacity == 0U ? 16U : *capacity;
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

static bool PhysicsWorld_AppendTrackedEntity(PhysicsWorld* world, struct Entity* entity)
{
    if (world == NULL || entity == NULL)
    {
        return false;
    }

    if (!PhysicsWorld_ReserveEntityPointerArray(&world->entities->tracked_entities, &world->entities->tracked_entity_capacity, world->entities->tracked_entity_count + 1U))
    {
        return false;
    }

    world->entities->tracked_entities[world->entities->tracked_entity_count++] = entity;
    return true;
}

static void PhysicsWorld_SyncEntities(PhysicsWorld* world, struct Scene* scene)
{
    size_t index = 0;

    if (world == NULL || scene == NULL)
    {
        return;
    }

    world->stats->active_entity_count = 0U;
    world->stats->dirty_entity_count = 0U;
    world->stats->collider_changed_entity_count = 0U;
    world->stats->body_create_count = 0U;
    world->stats->body_remove_count = 0U;
    world->stats->shape_change_count = 0U;

    for (index = 0; index < world->entities->tracked_entity_count; ++index)
    {
        Entity* entity = world->entities->tracked_entities[index];
        TransformComponent* transform = NULL;
        RigidBodyComponent* rigid_body = NULL;
        ColliderComponent* collider = NULL;
        bool needs_body_rebuild = false;
        bool has_valid_body = false;

        if (!Entity_IsActive(entity))
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
        rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
        collider = (ColliderComponent*)Entity_GetFixedComponent(entity, COMPONENT_COLLIDER);
        if (transform == NULL || rigid_body == NULL)
        {
            if (rigid_body != NULL && rigid_body->has_body && b2Body_IsValid(PhysicsWorld_LoadBodyHandle(rigid_body->body_id)))
            {
                PhysicsWorld_RemoveBodyForEntity(world, entity);
                world->stats->body_remove_count += 1U;
            }
            continue;
        }

        if (collider == NULL)
        {
            if (rigid_body->has_body && b2Body_IsValid(PhysicsWorld_LoadBodyHandle(rigid_body->body_id)))
            {
                PhysicsWorld_RemoveBodyForEntity(world, entity);
                world->stats->body_remove_count += 1U;
            }
            continue;
        }

        has_valid_body = rigid_body->has_body;
        if (!has_valid_body)
        {
            world->stats->body_create_count += 1U;
            if (!b2Body_IsValid(PhysicsWorld_EnsureEntityBody(world, entity)))
            {
                continue;
            }
            has_valid_body = true;
        }

        if (collider->dirty_flags != COLLIDER_DIRTY_NONE)
        {
            world->stats->collider_changed_entity_count += 1U;
            world->stats->shape_change_count += 1U;
            needs_body_rebuild = true;
        }

        if (rigid_body->dirty_flags != RIGID_BODY_DIRTY_NONE)
        {
            needs_body_rebuild = true;
        }

        if (needs_body_rebuild)
        {
            PhysicsWorld_RemoveBodyForEntity(world, entity);
            if (!b2Body_IsValid(PhysicsWorld_EnsureEntityBody(world, entity)))
            {
                continue;
            }
            has_valid_body = true;
            world->stats->dirty_entity_count += 1U;
        }

        if (transform->dirty && has_valid_body)
        {
            b2Body_SetTransform(PhysicsWorld_LoadBodyHandle(rigid_body->body_id), (b2Vec2){transform->x, transform->y}, b2MakeRot(transform->angle_radians));
            TransformComponent_ClearDirty(
                transform,
                TRANSFORM_DIRTY_ROTATION | TRANSFORM_DIRTY_SCALE
            );
            world->stats->dirty_entity_count += 1U;
        }

        if (has_valid_body)
        {
            world->stats->active_entity_count += 1U;
        }
    }
}

static void PhysicsWorld_SyncBodiesToTransforms(PhysicsWorld* world, struct Scene* scene)
{
    b2BodyEvents events;
    int index = 0;

    if (world == NULL || scene == NULL)
    {
        return;
    }

    if (!world->lifecycle->has_world)
    {
        return;
    }

    world->stats->moved_body_count = 0U;
    world->stats->awake_body_count = 0U;
    world->stats->sleeping_body_count = 0U;
    events = b2World_GetBodyEvents(world->lifecycle->world_id);
    for (index = 0; index < events.moveCount; ++index)
    {
        const b2BodyMoveEvent* event = events.moveEvents + index;
        Entity* entity = (Entity*)event->userData;
        TransformComponent* transform = NULL;
        RigidBodyComponent* rigid_body = NULL;

        if (!Entity_IsActive(entity))
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
        rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
        if (transform == NULL || rigid_body == NULL)
        {
            continue;
        }

        transform->previous_x = transform->x;
        transform->previous_y = transform->y;
        transform->previous_angle_radians = transform->angle_radians;
        transform->x = event->transform.p.x;
        transform->y = event->transform.p.y;
        transform->angle_radians = b2Rot_GetAngle(event->transform.q);
        TransformComponent_MarkDirty(transform, TRANSFORM_DIRTY_POSITION);
        world->stats->moved_body_count += 1U;
    }

    world->stats->awake_body_count = world->lifecycle->has_world ? (size_t)b2World_GetAwakeBodyCount(world->lifecycle->world_id) : 0U;
    world->stats->sleeping_body_count = world->stats->active_entity_count > world->stats->awake_body_count
        ? world->stats->active_entity_count - world->stats->awake_body_count
        : 0U;
}

void PhysicsWorld_Init(PhysicsWorld* world, const PhysicsWorldConfig* config)
{
    b2WorldDef world_def;
    if (world == NULL)
    {
        return;
    }

    memset(world, 0, sizeof(*world));
    world->lifecycle = (PhysicsWorldLifecycleState*)calloc(1U, sizeof(*world->lifecycle));
    world->tilemap = (PhysicsWorldTilemapState*)calloc(1U, sizeof(*world->tilemap));
    world->entities = (PhysicsWorldEntityState*)calloc(1U, sizeof(*world->entities));
    world->stats = (PhysicsWorldStatsState*)calloc(1U, sizeof(*world->stats));
    if (world->lifecycle == NULL || world->tilemap == NULL || world->entities == NULL || world->stats == NULL)
    {
        free(world->lifecycle);
        free(world->tilemap);
        free(world->entities);
        free(world->stats);
        memset(world, 0, sizeof(*world));
        return;
    }
    world_def = b2DefaultWorldDef();
    world->lifecycle->gravity_x = config != NULL ? config->gravity_x : 0.0f;
    world->lifecycle->gravity_y = config != NULL ? config->gravity_y : 1.0f;
    world->lifecycle->target_hz = config != NULL && config->target_hz > 0.0f ? config->target_hz : 30.0f;
    world->lifecycle->fixed_dt = 1.0f / world->lifecycle->target_hz;
    world->lifecycle->max_substeps = config != NULL && config->max_substeps > 0 ? config->max_substeps : 4u;
    world->lifecycle->base_step_substep_count = config != NULL && config->step_substep_count > 0 ? config->step_substep_count : 4u;
    world->lifecycle->step_substep_count = world->lifecycle->base_step_substep_count;
    world_def.gravity = (b2Vec2){world->lifecycle->gravity_x, world->lifecycle->gravity_y};
    world_def.enableSleep = true;
    if (config != NULL && config->task_system != NULL)
    {
        task_system_configure_box2d_world_def(config->task_system, &world_def);
    }
    world->lifecycle->world_id = b2CreateWorld(&world_def);
    world->lifecycle->has_world = b2World_IsValid(world->lifecycle->world_id);
}

PhysicsWorld* PhysicsWorld_Create(const PhysicsWorldConfig* config)
{
    PhysicsWorld* world = (PhysicsWorld*)calloc(1, sizeof(PhysicsWorld));
    if (world == NULL)
    {
        return NULL;
    }

    PhysicsWorld_Init(world, config);
    if (world->lifecycle == NULL || world->tilemap == NULL || world->entities == NULL || world->stats == NULL)
    {
        free(world);
        return NULL;
    }
    return world;
}

void PhysicsWorld_RegisterEntity(PhysicsWorld* world, struct Entity* entity)
{
    RigidBodyComponent* rigid_body = NULL;

    if (world == NULL || entity == NULL)
    {
        return;
    }

    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body == NULL)
    {
        return;
    }

    PhysicsWorld_AppendTrackedEntity(world, entity);
}

void PhysicsWorld_UnregisterEntity(PhysicsWorld* world, struct Entity* entity)
{
    size_t index = 0U;

    if (world == NULL || entity == NULL)
    {
        return;
    }

    for (index = 0U; index < world->entities->tracked_entity_count; ++index)
    {
        if (world->entities->tracked_entities[index] == entity)
        {
            memmove(
                &world->entities->tracked_entities[index],
                &world->entities->tracked_entities[index + 1U],
                sizeof(struct Entity*) * (world->entities->tracked_entity_count - index - 1U)
            );
            world->entities->tracked_entity_count -= 1U;
            world->entities->tracked_entities[world->entities->tracked_entity_count] = NULL;
            break;
        }
    }

    PhysicsWorld_RemoveBodyForEntity(world, entity);
}

void PhysicsWorld_GetStepConfig(const PhysicsWorld* world, float* out_fixed_dt, uint32_t* out_max_substeps)
{
    if (out_fixed_dt != NULL)
    {
        *out_fixed_dt = world != NULL && world->lifecycle->fixed_dt > 0.0f ? world->lifecycle->fixed_dt : 1.0f / 60.0f;
    }
    if (out_max_substeps != NULL)
    {
        *out_max_substeps = world != NULL && world->lifecycle->max_substeps > 0U ? world->lifecycle->max_substeps : 4U;
    }
}

void PhysicsWorld_Update(PhysicsWorld* world, struct Scene* scene, float dt_seconds)
{
    double step_started_ms = 0.0;

    if (world == NULL || scene == NULL || !world->lifecycle->has_world)
    {
        return;
    }

    PhysicsWorld_SyncEntities(world, scene);
    if (dt_seconds > 0.0f)
    {
        step_started_ms = PhysicsWorld_NowMs();
        b2World_Step(world->lifecycle->world_id, (float)dt_seconds, (int)world->lifecycle->step_substep_count);
        world->stats->last_box2d_step_wall_ms = PhysicsWorld_NowMs() - step_started_ms;
    }
    scene->stats->last_physics_substeps = dt_seconds > 0.0f ? 1u : 0u;
    PhysicsWorld_SyncBodiesToTransforms(world, scene);
}

struct Entity* PhysicsWorld_GetEntityForBody(PhysicsWorld* world, b2BodyId body_id)
{
    (void)world;
    if (!b2Body_IsValid(body_id))
    {
        return NULL;
    }

    return (struct Entity*)b2Body_GetUserData(body_id);
}

void PhysicsWorld_Destroy(PhysicsWorld* world)
{
    if (world == NULL)
    {
        return;
    }

    PhysicsWorld_ClearTilemapBodies(world);
    if (world->lifecycle != NULL && world->lifecycle->has_world)
    {
        b2DestroyWorld(world->lifecycle->world_id);
        world->lifecycle->has_world = false;
    }
    if (world->tilemap != NULL)
    {
        free(world->tilemap->tile_bodies);
    }
    if (world->entities != NULL)
    {
        free(world->entities->tracked_entities);
    }
    free(world->lifecycle);
    free(world->tilemap);
    free(world->entities);
    free(world->stats);
    free(world);
}
