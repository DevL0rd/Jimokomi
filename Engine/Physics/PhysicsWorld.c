#include "PhysicsWorldEntitiesInternal.h"
#include "PhysicsWorldLifecycleInternal.h"
#include "PhysicsWorldParticlesInternal.h"
#include "PhysicsWorldStatsInternal.h"
#include "PhysicsWorldTilemapInternal.h"

#include "PhysicsBodyControl.h"

#include "../Core/PlatformRuntimeInternal.h"
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

void task_system_configure_corephys_world_def(const struct TaskSystem* system, b2WorldDef* world_def);

#define PHYSICS_DEFAULT_MIN_HZ 15.0f
#define PHYSICS_DEFAULT_MAX_HZ 300.0f
#define PHYSICS_DEFAULT_FRAME_BUDGET_HZ 30.0f
#define PHYSICS_ADAPTIVE_RECOVERY_RATE 0.12f

static double PhysicsWorld_NowMs(void)
{
    return engine_platform_now_ms();
}

static float PhysicsWorld_ClampHz(float value, float min_hz, float max_hz)
{
    if (min_hz <= 0.0f)
    {
        min_hz = PHYSICS_DEFAULT_MIN_HZ;
    }
    if (max_hz < min_hz)
    {
        max_hz = min_hz;
    }
    return clamp_f(value, min_hz, max_hz);
}

static uint32_t PhysicsWorld_ClampTaskDelta(uint64_t after, uint64_t before)
{
    uint64_t delta = after >= before ? after - before : 0U;
    return delta > UINT32_MAX ? UINT32_MAX : (uint32_t)delta;
}

static void PhysicsWorld_SetTargetHz(PhysicsWorld* world, float target_hz)
{
    if (world == NULL || world->lifecycle == NULL)
    {
        return;
    }

    target_hz = PhysicsWorld_ClampHz(target_hz, world->lifecycle->min_hz, world->lifecycle->max_hz);
    world->lifecycle->target_hz = target_hz;
    world->lifecycle->fixed_dt = 1.0f / target_hz;
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

static bool PhysicsWorld_AppendOnscreenSleepEntity(PhysicsWorld* world, struct Entity* entity)
{
    if (world == NULL || entity == NULL)
    {
        return false;
    }

    if (!PhysicsWorld_ReserveEntityPointerArray(
            &world->entities->onscreen_sleep_entities,
            &world->entities->onscreen_sleep_entity_capacity,
            world->entities->onscreen_sleep_entity_count + 1U))
    {
        return false;
    }

    world->entities->onscreen_sleep_entities[world->entities->onscreen_sleep_entity_count++] = entity;
    return true;
}

static void PhysicsWorld_RemoveOnscreenSleepEntity(PhysicsWorld* world, struct Entity* entity)
{
    size_t index = 0U;

    if (world == NULL || world->entities == NULL || entity == NULL)
    {
        return;
    }

    for (index = 0U; index < world->entities->onscreen_sleep_entity_count; ++index)
    {
        if (world->entities->onscreen_sleep_entities[index] == entity)
        {
            memmove(
                &world->entities->onscreen_sleep_entities[index],
                &world->entities->onscreen_sleep_entities[index + 1U],
                sizeof(struct Entity*) * (world->entities->onscreen_sleep_entity_count - index - 1U)
            );
            world->entities->onscreen_sleep_entity_count -= 1U;
            world->entities->onscreen_sleep_entities[world->entities->onscreen_sleep_entity_count] = NULL;
            return;
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

void PhysicsWorld_UpdateAdaptiveStepRate(PhysicsWorld* world, float accumulator_seconds)
{
    float min_hz;
    float max_hz;
    float frame_budget_hz;
    float frame_budget_seconds;
    float measured_step_seconds;
    float delay_excess_seconds;
    float required_fixed_dt;
    float desired_hz;
    float current_hz;

    if (world == NULL || world->lifecycle == NULL || world->stats == NULL)
    {
        return;
    }

    world->stats->last_accumulator_seconds = accumulator_seconds > 0.0f ? accumulator_seconds : 0.0f;
    if (!world->lifecycle->adaptive_hz_enabled)
    {
        return;
    }

    measured_step_seconds = (float)world->stats->last_corephys_step_wall_ms / 1000.0f;
    if (measured_step_seconds <= 0.0f)
    {
        return;
    }

    min_hz = world->lifecycle->min_hz > 0.0f ? world->lifecycle->min_hz : PHYSICS_DEFAULT_MIN_HZ;
    max_hz = world->lifecycle->max_hz >= min_hz ? world->lifecycle->max_hz : min_hz;
    frame_budget_hz = world->lifecycle->frame_budget_hz > 0.0f
        ? world->lifecycle->frame_budget_hz
        : PHYSICS_DEFAULT_FRAME_BUDGET_HZ;
    frame_budget_seconds = 1.0f / frame_budget_hz;
    delay_excess_seconds = fmaxf(accumulator_seconds - frame_budget_seconds, 0.0f);
    required_fixed_dt = fmaxf(measured_step_seconds, delay_excess_seconds);
    if (required_fixed_dt <= 0.0f)
    {
        return;
    }

    desired_hz = PhysicsWorld_ClampHz(1.0f / required_fixed_dt, min_hz, max_hz);
    current_hz = PhysicsWorld_ClampHz(world->lifecycle->target_hz, min_hz, max_hz);
    if (desired_hz < current_hz)
    {
        PhysicsWorld_SetTargetHz(world, desired_hz);
    }
    else if (desired_hz > current_hz && accumulator_seconds <= frame_budget_seconds)
    {
        PhysicsWorld_SetTargetHz(
            world,
            current_hz + ((desired_hz - current_hz) * PHYSICS_ADAPTIVE_RECOVERY_RATE)
        );
    }
}

void PhysicsWorld_SetSleepThreshold(PhysicsWorld* world, float sleep_threshold)
{
    size_t index = 0U;

    if (world == NULL || world->lifecycle == NULL || world->entities == NULL)
    {
        return;
    }
    if (sleep_threshold < 0.0f)
    {
        sleep_threshold = 0.0f;
    }

    world->lifecycle->sleep_threshold = sleep_threshold;
    world->lifecycle->has_sleep_threshold_setting = true;
    for (index = 0U; index < world->entities->tracked_entity_count; ++index)
    {
        RigidBodyComponent* rigid_body = NULL;
        b2BodyId body_id;
        Entity* entity = world->entities->tracked_entities[index];

        if (entity == NULL)
        {
            continue;
        }
        rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
        if (rigid_body == NULL || !rigid_body->has_body)
        {
            continue;
        }
        body_id = PhysicsWorld_LoadBodyHandle(rigid_body->body_id);
        if (b2Body_IsValid(body_id))
        {
            b2Body_SetSleepThreshold(body_id, sleep_threshold);
            rigid_body->applied_sleep_threshold = sleep_threshold;
            rigid_body->sleep_threshold_is_onscreen = false;
        }
    }
    world->entities->onscreen_sleep_entity_count = 0U;
}

void PhysicsWorld_UpdateViewSleepThresholds(
    PhysicsWorld* world,
    struct Entity* const* onscreen_entities,
    size_t onscreen_entity_count,
    float onscreen_sleep_threshold,
    float offscreen_sleep_threshold
) {
    uint32_t stamp;
    size_t index = 0U;
    size_t compacted_count = 0U;

    if (world == NULL || world->lifecycle == NULL || world->entities == NULL)
    {
        return;
    }
    if (onscreen_sleep_threshold < 0.0f)
    {
        onscreen_sleep_threshold = 0.0f;
    }
    if (offscreen_sleep_threshold < 0.0f)
    {
        offscreen_sleep_threshold = 0.0f;
    }

    world->lifecycle->sleep_threshold = offscreen_sleep_threshold;
    world->lifecycle->has_sleep_threshold_setting = true;
    world->entities->sleep_visibility_stamp += 1U;
    if (world->entities->sleep_visibility_stamp == 0U)
    {
        world->entities->sleep_visibility_stamp = 1U;
    }
    stamp = world->entities->sleep_visibility_stamp;

    for (index = 0U; index < onscreen_entity_count; ++index)
    {
        Entity* entity = onscreen_entities != NULL ? onscreen_entities[index] : NULL;
        RigidBodyComponent* rigid_body = NULL;
        bool was_onscreen = false;

        if (entity == NULL)
        {
            continue;
        }

        rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
        if (rigid_body == NULL || rigid_body->body_type == RIGID_BODY_STATIC)
        {
            continue;
        }

        was_onscreen = rigid_body->sleep_threshold_is_onscreen;
        rigid_body->sleep_visibility_stamp = stamp;
        if (PhysicsWorld_SetEntitySleepThreshold(world, entity, onscreen_sleep_threshold, true) && !was_onscreen)
        {
            (void)PhysicsWorld_AppendOnscreenSleepEntity(world, entity);
        }
    }

    for (index = 0U; index < world->entities->onscreen_sleep_entity_count; ++index)
    {
        Entity* entity = world->entities->onscreen_sleep_entities[index];
        RigidBodyComponent* rigid_body = NULL;

        if (entity == NULL)
        {
            continue;
        }

        rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
        if (rigid_body == NULL || rigid_body->sleep_visibility_stamp != stamp)
        {
            (void)PhysicsWorld_SetEntitySleepThreshold(world, entity, offscreen_sleep_threshold, false);
            continue;
        }

        world->entities->onscreen_sleep_entities[compacted_count++] = entity;
    }

    for (index = compacted_count; index < world->entities->onscreen_sleep_entity_count; ++index)
    {
        world->entities->onscreen_sleep_entities[index] = NULL;
    }
    world->entities->onscreen_sleep_entity_count = compacted_count;
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
    world->particles = (PhysicsWorldParticleState*)calloc(1U, sizeof(*world->particles));
    if (world->lifecycle == NULL ||
        world->tilemap == NULL ||
        world->entities == NULL ||
        world->stats == NULL ||
        world->particles == NULL)
    {
        free(world->lifecycle);
        free(world->tilemap);
        free(world->entities);
        free(world->stats);
        free(world->particles);
        memset(world, 0, sizeof(*world));
        return;
    }
    world_def = b2DefaultWorldDef();
    world->lifecycle->gravity_x = config != NULL ? config->gravity_x : 0.0f;
    world->lifecycle->gravity_y = config != NULL ? config->gravity_y : 1.0f;
    world->lifecycle->min_hz = config != NULL && config->min_hz > 0.0f ? config->min_hz : PHYSICS_DEFAULT_MIN_HZ;
    world->lifecycle->max_hz = config != NULL && config->max_hz >= world->lifecycle->min_hz
        ? config->max_hz
        : PHYSICS_DEFAULT_MAX_HZ;
    world->lifecycle->frame_budget_hz = config != NULL && config->frame_budget_hz > 0.0f
        ? config->frame_budget_hz
        : PHYSICS_DEFAULT_FRAME_BUDGET_HZ;
    world->lifecycle->adaptive_hz_enabled =
        config == NULL || !config->has_adaptive_hz_setting || config->adaptive_hz_enabled;
    world->lifecycle->sleep_threshold = config != NULL && config->sleep_threshold >= 0.0f
        ? config->sleep_threshold
        : 0.0f;
    world->lifecycle->has_sleep_threshold_setting = config != NULL && config->has_sleep_threshold_setting;
    PhysicsWorld_SetTargetHz(
        world,
        config != NULL && config->target_hz > 0.0f ? config->target_hz : world->lifecycle->max_hz
    );
    world->lifecycle->max_substeps = config != NULL && config->max_substeps > 0 ? config->max_substeps : 4u;
    world->lifecycle->base_step_substep_count = config != NULL && config->step_substep_count > 0 ? config->step_substep_count : 4u;
    world->lifecycle->step_substep_count = world->lifecycle->base_step_substep_count;
    world_def.gravity = (b2Vec2){world->lifecycle->gravity_x, world->lifecycle->gravity_y};
    world_def.enableSleep = true;
    if (config != NULL && config->has_continuous_collision_setting)
    {
        world_def.enableContinuous = config->continuous_collision_enabled;
    }
    world->lifecycle->task_system = config != NULL ? config->task_system : NULL;
    if (config != NULL && config->task_system != NULL)
    {
        task_system_configure_corephys_world_def(config->task_system, &world_def);
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
    if (world->lifecycle == NULL ||
        world->tilemap == NULL ||
        world->entities == NULL ||
        world->stats == NULL ||
        world->particles == NULL)
    {
        free(world);
        return NULL;
    }
    return world;
}

void PhysicsWorld_RegisterEntity(PhysicsWorld* world, struct Entity* entity)
{
    RigidBodyComponent* rigid_body = NULL;
    b2BodyId body_id;

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
    body_id = PhysicsWorld_EnsureEntityBody(world, entity);
    if (b2Body_IsValid(body_id))
    {
        world->stats->body_create_count += 1U;
    }
    world->stats->active_entity_count = world->entities->tracked_entity_count;
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
            world->stats->active_entity_count = world->entities->tracked_entity_count;
            break;
        }
    }

    PhysicsWorld_RemoveOnscreenSleepEntity(world, entity);
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
    TaskSystemStatsSnapshot task_stats_before;
    TaskSystemStatsSnapshot task_stats_after;
    bool has_task_stats = false;

    if (world == NULL || scene == NULL || !world->lifecycle->has_world)
    {
        return;
    }

    if (dt_seconds > 0.0f)
    {
        if (world->lifecycle->task_system != NULL)
        {
            task_system_get_stats_snapshot(world->lifecycle->task_system, &task_stats_before);
            has_task_stats = true;
        }

        step_started_ms = PhysicsWorld_NowMs();
        b2World_Step(world->lifecycle->world_id, (float)dt_seconds, (int)world->lifecycle->step_substep_count);
        world->stats->last_corephys_step_wall_ms = PhysicsWorld_NowMs() - step_started_ms;

        if (has_task_stats)
        {
            task_system_get_stats_snapshot(world->lifecycle->task_system, &task_stats_after);
            world->stats->task_worker_count = task_stats_after.worker_count > 0 ? (uint32_t)task_stats_after.worker_count : 0U;
            world->stats->task_background_thread_count =
                task_stats_after.background_thread_count > 0 ? (uint32_t)task_stats_after.background_thread_count : 0U;
            world->stats->corephys_enqueued_task_count =
                PhysicsWorld_ClampTaskDelta(task_stats_after.enqueued_task_count, task_stats_before.enqueued_task_count);
            world->stats->corephys_inline_task_count =
                PhysicsWorld_ClampTaskDelta(task_stats_after.inline_task_count, task_stats_before.inline_task_count);
            world->stats->corephys_main_chunk_count =
                PhysicsWorld_ClampTaskDelta(task_stats_after.main_chunk_count, task_stats_before.main_chunk_count);
            world->stats->corephys_worker_chunk_count =
                PhysicsWorld_ClampTaskDelta(task_stats_after.worker_chunk_count, task_stats_before.worker_chunk_count);
        }
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
    PhysicsWorld_DestroyParticleState(world);
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
        free(world->entities->onscreen_sleep_entities);
    }
    free(world->lifecycle);
    free(world->tilemap);
    free(world->entities);
    free(world->stats);
    free(world);
}
