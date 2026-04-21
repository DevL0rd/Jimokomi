#include "PhysicsWorldInternal.h"


#include "../Core/TaskSystem.h"
#include "../Scene/SceneInternal.h"
#include "../Scene/EntityInternal.h"
#include "../Scene/Components/ColliderComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Components/TransformComponent.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void task_system_configure_box2d_world_def(const struct TaskSystem* system, b2WorldDef* world_def);
static struct Entity* PhysicsWorld_GetEntityForBody(PhysicsWorld* world, b2BodyId body_id);

static double PhysicsWorld_NowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static b2BodyId PhysicsWorld_LoadBodyHandle(PhysicsBodyHandle handle)
{
    return (b2BodyId){ handle.index1, handle.world0, handle.generation };
}

static PhysicsBodyHandle PhysicsWorld_StoreBodyHandle(b2BodyId body_id)
{
    return (PhysicsBodyHandle){ body_id.index1, body_id.world0, body_id.generation };
}

static PhysicsShapeHandle PhysicsWorld_StoreShapeHandle(b2ShapeId shape_id)
{
    return (PhysicsShapeHandle){ shape_id.index1, shape_id.world0, shape_id.generation };
}

static b2BodyType PhysicsWorld_ToBox2DType(RigidBodyType type)
{
    switch (type)
    {
        case RIGID_BODY_STATIC:
            return b2_staticBody;
        case RIGID_BODY_KINEMATIC:
            return b2_kinematicBody;
        case RIGID_BODY_DYNAMIC:
        default:
            return b2_dynamicBody;
    }
}

typedef struct PhysicsQueryAccumulator
{
    PhysicsWorld* world;
    PhysicsQueryResult* results;
    size_t capacity;
    size_t count;
    b2Vec2 ray_origin;
} PhysicsQueryAccumulator;

static float PhysicsWorld_ClampFloat(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static float PhysicsWorld_LerpFloat(float a, float b, float alpha)
{
    return a + ((b - a) * alpha);
}

static bool PhysicsWorld_ReserveTileBodies(PhysicsWorld* world, size_t required_capacity)
{
    b2BodyId* bodies = NULL;
    size_t new_capacity = 0;

    if (world == NULL)
    {
        return false;
    }

    if (world->tile_body_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = world->tile_body_capacity == 0 ? 8 : world->tile_body_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    bodies = (b2BodyId*)realloc(world->tile_bodies, sizeof(b2BodyId) * new_capacity);
    if (bodies == NULL)
    {
        return false;
    }

    world->tile_bodies = bodies;
    world->tile_body_capacity = new_capacity;
    return true;
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

    if (!PhysicsWorld_ReserveEntityPointerArray(&world->tracked_entities, &world->tracked_entity_capacity, world->tracked_entity_count + 1U))
    {
        return false;
    }

    world->tracked_entities[world->tracked_entity_count++] = entity;
    return true;
}

static size_t PhysicsWorld_FindNearestLevelIndex(const PhysicsWorld* world, float target_hz)
{
    size_t best_index = 0;
    float best_distance = 1.0e30f;
    size_t index = 0;

    if (world == NULL || world->adaptive_level_count == 0)
    {
        return 0;
    }

    for (index = 0; index < world->adaptive_level_count; ++index)
    {
        float distance = fabsf(world->adaptive_levels[index] - target_hz);
        if (distance < best_distance)
        {
            best_distance = distance;
            best_index = index;
        }
    }

    return best_index;
}

static uint32_t PhysicsWorld_ComputeAdaptiveStepSubstepsForHz(const PhysicsWorld* world, float target_hz)
{
    uint32_t base_substeps = 4u;

    if (world != NULL)
    {
        base_substeps = world->base_step_substep_count > 0u ? world->base_step_substep_count : 4u;
    }

    if (target_hz >= 90.0f)
    {
        return base_substeps;
    }
    if (target_hz >= 45.0f)
    {
        return base_substeps > 2u ? 2u : base_substeps;
    }
    return 1u;
}

static int PhysicsWorld_GetTileValue(const PhysicsWorld* world, int tile_x, int tile_y, int layer_index)
{
    if (world == NULL || world->tilemap_adapter == NULL || world->tilemap == NULL || world->tilemap_adapter->get_tile == NULL)
    {
        return 0;
    }

    return world->tilemap_adapter->get_tile(world->tilemap, tile_x, tile_y, layer_index);
}

static bool PhysicsWorld_IsTileSolid(const PhysicsWorld* world, int tile_value)
{
    if (world == NULL || tile_value < 0 || (size_t)tile_value >= world->tile_rule_count || world->tile_rules == NULL)
    {
        return false;
    }

    return world->tile_rules[tile_value].solid;
}

static void PhysicsWorld_ClearTilemapBodies(PhysicsWorld* world)
{
    size_t index = 0;

    if (world == NULL || !world->has_world)
    {
        return;
    }

    for (index = 0; index < world->tile_body_count; ++index)
    {
        if (b2Body_IsValid(world->tile_bodies[index]))
        {
            b2DestroyBody(world->tile_bodies[index]);
        }
    }
    world->tile_body_count = 0;
}

static void PhysicsWorld_RebuildTilemapBodies(PhysicsWorld* world)
{
    int width_tiles = 0;
    int height_tiles = 0;
    int tile_width = 0;
    int tile_height = 0;
    int tile_y = 0;

    PhysicsWorld_ClearTilemapBodies(world);

    if (world == NULL || world->tilemap_adapter == NULL || world->tilemap == NULL)
    {
        return;
    }

    if (world->tilemap_adapter->get_width_tiles == NULL || world->tilemap_adapter->get_height_tiles == NULL ||
        world->tilemap_adapter->get_tile_width == NULL || world->tilemap_adapter->get_tile_height == NULL)
    {
        return;
    }

    width_tiles = world->tilemap_adapter->get_width_tiles(world->tilemap);
    height_tiles = world->tilemap_adapter->get_height_tiles(world->tilemap);
    tile_width = world->tilemap_adapter->get_tile_width(world->tilemap);
    tile_height = world->tilemap_adapter->get_tile_height(world->tilemap);

    for (tile_y = 0; tile_y < height_tiles; ++tile_y)
    {
        int tile_x = 0;
        int run_start_x = -1;
        for (tile_x = 0; tile_x <= width_tiles; ++tile_x)
        {
            int tile_value = 0;
            bool is_solid = false;
            if (tile_x < width_tiles)
            {
                tile_value = PhysicsWorld_GetTileValue(world, tile_x, tile_y, 0);
                is_solid = PhysicsWorld_IsTileSolid(world, tile_value);
            }

            if (is_solid)
            {
                if (run_start_x < 0)
                {
                    run_start_x = tile_x;
                }
            }
            else if (run_start_x >= 0)
            {
                int run_end_x = tile_x - 1;
                int run_width = (run_end_x - run_start_x) + 1;
                b2BodyDef body_def = b2DefaultBodyDef();
                b2ShapeDef shape_def = b2DefaultShapeDef();
                b2Polygon box = b2MakeBox((run_width * tile_width) * 0.5f, tile_height * 0.5f);
                b2BodyId body_id;

                body_def.type = b2_staticBody;
                body_def.position.x = (run_start_x * tile_width) + (run_width * tile_width * 0.5f);
                body_def.position.y = (tile_y * tile_height) + (tile_height * 0.5f);
                body_id = b2CreateBody(world->world_id, &body_def);
                shape_def.material.friction = 1.0f;
                shape_def.material.restitution = 0.0f;
                b2CreatePolygonShape(body_id, &shape_def, &box);

                if (PhysicsWorld_ReserveTileBodies(world, world->tile_body_count + 1))
                {
                    world->tile_bodies[world->tile_body_count++] = body_id;
                }
                run_start_x = -1;
            }
        }
    }
}

static b2BodyId PhysicsWorld_EnsureEntityBody(PhysicsWorld* world, struct Entity* entity)
{
    TransformComponent* transform = NULL;
    RigidBodyComponent* rigid_body = NULL;
    ColliderComponent* collider = NULL;

    if (world == NULL || entity == NULL || !world->has_world)
    {
        return b2_nullBodyId;
    }

    transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
    rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
    collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
    if (transform == NULL || rigid_body == NULL || collider == NULL)
    {
        PhysicsWorld_RemoveBodyForEntity(world, entity);
        return b2_nullBodyId;
    }

    if (rigid_body->has_body && b2Body_IsValid(PhysicsWorld_LoadBodyHandle(rigid_body->body_id)))
    {
        return PhysicsWorld_LoadBodyHandle(rigid_body->body_id);
    }

    {
        b2BodyDef body_def = b2DefaultBodyDef();
        b2ShapeDef shape_def = b2DefaultShapeDef();
        b2BodyId body_id;

        body_def.type = PhysicsWorld_ToBox2DType(rigid_body->body_type);
        body_def.position.x = transform->x;
        body_def.position.y = transform->y;
        body_def.rotation = b2MakeRot(transform->angle_radians);
        body_def.linearVelocity.x = rigid_body->initial_velocity_x;
        body_def.linearVelocity.y = rigid_body->initial_velocity_y;
        body_def.angularVelocity = rigid_body->initial_angular_velocity;
        body_def.linearDamping = rigid_body->friction_air;
        body_def.angularDamping = rigid_body->friction_air;
        body_def.userData = entity;
        body_def.enableSleep = true;
        body_def.motionLocks.angularZ = rigid_body->fixed_rotation;

        body_id = b2CreateBody(world->world_id, &body_def);
        shape_def.density = rigid_body->density;
        shape_def.material.friction = rigid_body->friction;
        shape_def.material.restitution = rigid_body->restitution;
        shape_def.isSensor = collider->is_sensor;
        shape_def.userData = entity;

        if (collider->shape == COLLIDER_SHAPE_RECT)
        {
            b2Polygon box = b2MakeBox(collider->width * 0.5f, collider->height * 0.5f);
            rigid_body->shape_id = PhysicsWorld_StoreShapeHandle(b2CreatePolygonShape(body_id, &shape_def, &box));
        }
        else
        {
            b2Circle circle;
            circle.center = (b2Vec2){0.0f, 0.0f};
            circle.radius = collider->radius;
            rigid_body->shape_id = PhysicsWorld_StoreShapeHandle(b2CreateCircleShape(body_id, &shape_def, &circle));
        }

        rigid_body->body_id = PhysicsWorld_StoreBodyHandle(body_id);
        rigid_body->has_body = true;
        RigidBodyComponent_ClearDirty(rigid_body, RIGID_BODY_DIRTY_DEFINITION);
        ColliderComponent_ClearDirty(collider, COLLIDER_DIRTY_SHAPE | COLLIDER_DIRTY_BOUNDS);
        return body_id;
    }
}

static bool PhysicsWorld_GetEntityBody(
    PhysicsWorld* world,
    struct Entity* entity,
    bool create_if_missing,
    b2BodyId* out_body_id
) {
    RigidBodyComponent* rigid_body;
    b2BodyId body_id;

    if (out_body_id != NULL) {
        *out_body_id = b2_nullBodyId;
    }
    if (world == NULL || entity == NULL || out_body_id == NULL) {
        return false;
    }

    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body == NULL) {
        return false;
    }

    body_id = rigid_body->has_body ? PhysicsWorld_LoadBodyHandle(rigid_body->body_id) : b2_nullBodyId;
    if (!b2Body_IsValid(body_id) && create_if_missing) {
        body_id = PhysicsWorld_EnsureEntityBody(world, entity);
    }
    if (!b2Body_IsValid(body_id)) {
        return false;
    }

    *out_body_id = body_id;
    return true;
}

static void PhysicsWorld_SyncEntities(PhysicsWorld* world, struct Scene* scene)
{
    size_t index = 0;

    if (world == NULL || scene == NULL)
    {
        return;
    }

    world->active_entity_count = 0U;
    world->dirty_entity_count = 0U;
    world->collider_changed_entity_count = 0U;
    world->body_create_count = 0U;
    world->body_remove_count = 0U;
    world->shape_change_count = 0U;

    for (index = 0; index < world->tracked_entity_count; ++index)
    {
        Entity* entity = world->tracked_entities[index];
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
                world->body_remove_count += 1U;
            }
            continue;
        }

        if (collider == NULL)
        {
            if (rigid_body->has_body && b2Body_IsValid(PhysicsWorld_LoadBodyHandle(rigid_body->body_id)))
            {
                PhysicsWorld_RemoveBodyForEntity(world, entity);
                world->body_remove_count += 1U;
            }
            continue;
        }

        has_valid_body = rigid_body->has_body;
        if (!has_valid_body)
        {
            world->body_create_count += 1U;
            if (!b2Body_IsValid(PhysicsWorld_EnsureEntityBody(world, entity)))
            {
                continue;
            }
            has_valid_body = true;
        }

        if (collider->dirty_flags != COLLIDER_DIRTY_NONE)
        {
            world->collider_changed_entity_count += 1U;
            world->shape_change_count += 1U;
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
            world->dirty_entity_count += 1U;
        }

        if (transform->dirty && has_valid_body)
        {
            b2Body_SetTransform(PhysicsWorld_LoadBodyHandle(rigid_body->body_id), (b2Vec2){transform->x, transform->y}, b2MakeRot(transform->angle_radians));
            TransformComponent_ClearDirty(
                transform,
                TRANSFORM_DIRTY_ROTATION | TRANSFORM_DIRTY_SCALE
            );
            world->dirty_entity_count += 1U;
        }

        if (has_valid_body)
        {
            world->active_entity_count += 1U;
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

    if (!world->has_world)
    {
        return;
    }

    world->moved_body_count = 0U;
    world->awake_body_count = 0U;
    world->sleeping_body_count = 0U;
    events = b2World_GetBodyEvents(world->world_id);
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
        world->moved_body_count += 1U;
    }

    world->awake_body_count = world->has_world ? (size_t)b2World_GetAwakeBodyCount(world->world_id) : 0U;
    world->sleeping_body_count = world->active_entity_count > world->awake_body_count
        ? world->active_entity_count - world->awake_body_count
        : 0U;
}

static bool PhysicsWorld_OverlapCollector(b2ShapeId shape_id, void* context)
{
    PhysicsQueryAccumulator* accumulator = (PhysicsQueryAccumulator*)context;
    b2BodyId body_id;

    if (accumulator == NULL || accumulator->count >= accumulator->capacity)
    {
        return false;
    }

    body_id = b2Shape_GetBody(shape_id);
    accumulator->results[accumulator->count++] = (PhysicsQueryResult){
        PhysicsWorld_GetEntityForBody(accumulator->world, body_id),
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        0.0f
    };
    return true;
}

static float PhysicsWorld_RayCollector(b2ShapeId shape_id, b2Vec2 point, b2Vec2 normal, float fraction, void* context)
{
    PhysicsQueryAccumulator* accumulator = (PhysicsQueryAccumulator*)context;
    b2BodyId body_id;

    if (accumulator == NULL || accumulator->count >= accumulator->capacity)
    {
        return 0.0f;
    }

    body_id = b2Shape_GetBody(shape_id);
    accumulator->results[accumulator->count++] = (PhysicsQueryResult){
        PhysicsWorld_GetEntityForBody(accumulator->world, body_id),
        { point.x, point.y },
        { normal.x, normal.y },
        fraction
    };
    return 1.0f;
}

void PhysicsWorld_Init(PhysicsWorld* world, const PhysicsWorldConfig* config)
{
    b2WorldDef world_def;
    float default_levels[] = {120.0f, 90.0f, 75.0f, 60.0f, 45.0f, 30.0f, 20.0f, 15.0f};
    size_t index = 0;

    if (world == NULL)
    {
        return;
    }

    memset(world, 0, sizeof(*world));
    world_def = b2DefaultWorldDef();
    world->gravity_x = config != NULL ? config->gravity_x : 0.0f;
    world->gravity_y = config != NULL ? config->gravity_y : 1.0f;
    world->target_hz = config != NULL && config->target_hz > 0.0f ? config->target_hz : 30.0f;
    world->fixed_dt = 1.0f / world->target_hz;
    world->max_substeps = config != NULL && config->max_substeps > 0 ? config->max_substeps : 4u;
    world->base_step_substep_count = config != NULL && config->step_substep_count > 0 ? config->step_substep_count : 4u;
    world->step_substep_count = world->base_step_substep_count;
    world->adaptive_enabled = config != NULL && config->adaptive_enabled;
    world->adaptive_budget_ms = config != NULL && config->downshift_frame_ms > 0.0f ? config->downshift_frame_ms : 40.0f;
    world->adaptive_recovery_ms = config != NULL && config->upshift_frame_ms > 0.0f ? config->upshift_frame_ms : world->adaptive_budget_ms * 0.5f;
    world->adaptive_blend_alpha = 0.24f;
    strncpy(world->last_tuner_reason, "init", sizeof(world->last_tuner_reason) - 1);

    if (config != NULL && config->adaptive_levels != NULL && config->adaptive_level_count > 0)
    {
        world->adaptive_level_count = config->adaptive_level_count > 8 ? 8 : config->adaptive_level_count;
        for (index = 0; index < world->adaptive_level_count; ++index)
        {
            world->adaptive_levels[index] = config->adaptive_levels[index];
        }
    }
    else
    {
        world->adaptive_level_count = sizeof(default_levels) / sizeof(default_levels[0]);
        for (index = 0; index < world->adaptive_level_count; ++index)
        {
            world->adaptive_levels[index] = default_levels[index];
        }
    }

    world->adaptive_min_hz = world->target_hz;
    world->adaptive_max_hz = world->target_hz;
    if (world->adaptive_level_count > 0u)
    {
        world->adaptive_max_hz = world->adaptive_levels[0];
        world->adaptive_min_hz = world->adaptive_levels[0];
        for (index = 1; index < world->adaptive_level_count; ++index)
        {
            if (world->adaptive_levels[index] > world->adaptive_max_hz)
            {
                world->adaptive_max_hz = world->adaptive_levels[index];
            }
            if (world->adaptive_levels[index] < world->adaptive_min_hz)
            {
                world->adaptive_min_hz = world->adaptive_levels[index];
            }
        }
    }
    world->target_hz = PhysicsWorld_ClampFloat(world->target_hz, world->adaptive_min_hz, world->adaptive_max_hz);
    world->fixed_dt = 1.0f / world->target_hz;
    world->step_substep_count = PhysicsWorld_ComputeAdaptiveStepSubstepsForHz(world, world->target_hz);
    world->current_level_index = (uint32_t)PhysicsWorld_FindNearestLevelIndex(world, world->target_hz);
    world_def.gravity = (b2Vec2){world->gravity_x, world->gravity_y};
    world_def.enableSleep = true;
    if (config != NULL && config->task_system != NULL)
    {
        task_system_configure_box2d_world_def(config->task_system, &world_def);
    }
    world->world_id = b2CreateWorld(&world_def);
    world->has_world = b2World_IsValid(world->world_id);
}

PhysicsWorld* PhysicsWorld_Create(const PhysicsWorldConfig* config)
{
    PhysicsWorld* world = (PhysicsWorld*)calloc(1, sizeof(PhysicsWorld));
    if (world == NULL)
    {
        return NULL;
    }

    PhysicsWorld_Init(world, config);
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

    for (index = 0U; index < world->tracked_entity_count; ++index)
    {
        if (world->tracked_entities[index] == entity)
        {
            memmove(
                &world->tracked_entities[index],
                &world->tracked_entities[index + 1U],
                sizeof(struct Entity*) * (world->tracked_entity_count - index - 1U)
            );
            world->tracked_entity_count -= 1U;
            world->tracked_entities[world->tracked_entity_count] = NULL;
            break;
        }
    }

    PhysicsWorld_RemoveBodyForEntity(world, entity);
}

void PhysicsWorld_SetTargetHz(PhysicsWorld* world, float target_hz, const char* reason)
{
    if (world == NULL)
    {
        return;
    }

    if (world->adaptive_enabled)
    {
        target_hz = PhysicsWorld_ClampFloat(target_hz, world->adaptive_min_hz, world->adaptive_max_hz);
    }

    world->target_hz = target_hz < 1.0f ? 1.0f : target_hz;
    world->fixed_dt = 1.0f / world->target_hz;
    world->step_substep_count = PhysicsWorld_ComputeAdaptiveStepSubstepsForHz(world, world->target_hz);
    world->current_level_index = (uint32_t)PhysicsWorld_FindNearestLevelIndex(world, world->target_hz);
    if (reason != NULL)
    {
        memset(world->last_tuner_reason, 0, sizeof(world->last_tuner_reason));
        strncpy(world->last_tuner_reason, reason, sizeof(world->last_tuner_reason) - 1);
    }
}

void PhysicsWorld_UpdateAdaptiveBudget(PhysicsWorld* world, float frame_ms)
{
    float desired_hz = 0.0f;
    float clamped_hz = 0.0f;
    const char* reason = "hold";

    if (world == NULL || !world->adaptive_enabled || frame_ms <= 0.0f)
    {
        return;
    }

    desired_hz = world->target_hz;
    if (frame_ms > world->adaptive_budget_ms)
    {
        desired_hz = world->target_hz * (world->adaptive_budget_ms / frame_ms);
        reason = "budget";
    }
    else if (frame_ms < world->adaptive_recovery_ms)
    {
        float safe_frame_ms = frame_ms < 0.25f ? 0.25f : frame_ms;
        desired_hz = world->target_hz * (world->adaptive_recovery_ms / safe_frame_ms);
        reason = "recover";
    }
    else
    {
        reason = "hold";
    }

    clamped_hz = PhysicsWorld_ClampFloat(desired_hz, world->adaptive_min_hz, world->adaptive_max_hz);
    PhysicsWorld_SetTargetHz(
        world,
        PhysicsWorld_LerpFloat(world->target_hz, clamped_hz, world->adaptive_blend_alpha),
        reason
    );
}

void PhysicsWorld_GetStepConfig(const PhysicsWorld* world, float* out_fixed_dt, uint32_t* out_max_substeps)
{
    if (out_fixed_dt != NULL)
    {
        *out_fixed_dt = world != NULL && world->fixed_dt > 0.0f ? world->fixed_dt : 1.0f / 60.0f;
    }
    if (out_max_substeps != NULL)
    {
        *out_max_substeps = world != NULL && world->max_substeps > 0U ? world->max_substeps : 4U;
    }
}

void PhysicsWorld_RemoveBodyForEntity(PhysicsWorld* world, struct Entity* entity)
{
    RigidBodyComponent* rigid_body = NULL;

    if (world == NULL || entity == NULL)
    {
        return;
    }

    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body == NULL || !rigid_body->has_body || !b2Body_IsValid(PhysicsWorld_LoadBodyHandle(rigid_body->body_id)))
    {
        return;
    }

    b2DestroyBody(PhysicsWorld_LoadBodyHandle(rigid_body->body_id));
    rigid_body->body_id = (PhysicsBodyHandle){ 0 };
    rigid_body->shape_id = (PhysicsShapeHandle){ 0 };
    rigid_body->has_body = false;
    RigidBodyComponent_MarkDefinitionDirty(rigid_body);
}

void PhysicsWorld_ClearEntityBodies(PhysicsWorld* world, struct Scene* scene)
{
    size_t index = 0;

    if (world == NULL)
    {
        return;
    }

    for (index = 0; index < world->tracked_entity_count; ++index)
    {
        PhysicsWorld_RemoveBodyForEntity(world, world->tracked_entities[index]);
    }

    (void)scene;
}

void PhysicsWorld_SetEntityPosition(PhysicsWorld* world, struct Entity* entity, float x, float y)
{
    TransformComponent* transform = NULL;
    RigidBodyComponent* rigid_body = NULL;
    ColliderComponent* collider = NULL;

    if (entity == NULL)
    {
        return;
    }

    transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
    if (transform == NULL)
    {
        return;
    }

    transform->previous_x = x;
    transform->previous_y = y;
    transform->previous_angle_radians = transform->angle_radians;
    TransformComponent_SetPosition(transform, x, y, true);

    rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
    collider = (ColliderComponent*)Entity_GetFixedComponent(entity, COMPONENT_COLLIDER);
    if (world != NULL && rigid_body != NULL && collider != NULL)
    {
        b2BodyId body_id = rigid_body->has_body ? PhysicsWorld_LoadBodyHandle(rigid_body->body_id) : PhysicsWorld_EnsureEntityBody(world, entity);
        if (b2Body_IsValid(body_id))
        {
            b2Body_SetTransform(body_id, (b2Vec2){x, y}, b2MakeRot(transform->angle_radians));
            b2Body_SetLinearVelocity(body_id, (b2Vec2){0.0f, 0.0f});
            b2Body_SetAngularVelocity(body_id, 0.0f);
            return;
        }
    }

    TransformComponent_MarkDirty(transform, TRANSFORM_DIRTY_POSITION | TRANSFORM_DIRTY_TELEPORT);
}

bool PhysicsWorld_GetEntityLinearVelocity(PhysicsWorld* world, struct Entity* entity, Vec2* out_velocity)
{
    b2BodyId body_id;
    b2Vec2 velocity;

    if (out_velocity != NULL) {
        out_velocity->x = 0.0f;
        out_velocity->y = 0.0f;
    }
    if (out_velocity == NULL || !PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    velocity = b2Body_GetLinearVelocity(body_id);
    out_velocity->x = velocity.x;
    out_velocity->y = velocity.y;
    return true;
}

bool PhysicsWorld_SetEntityLinearVelocity(PhysicsWorld* world, struct Entity* entity, Vec2 velocity)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_SetLinearVelocity(body_id, (b2Vec2){ velocity.x, velocity.y });
    return true;
}

bool PhysicsWorld_SetEntityAngularVelocity(PhysicsWorld* world, struct Entity* entity, float angular_velocity)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_SetAngularVelocity(body_id, angular_velocity);
    return true;
}

bool PhysicsWorld_ApplyEntityForce(PhysicsWorld* world, struct Entity* entity, Vec2 force, bool wake)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_ApplyForceToCenter(body_id, (b2Vec2){ force.x, force.y }, wake);
    return true;
}

bool PhysicsWorld_ApplyEntityLinearImpulse(PhysicsWorld* world, struct Entity* entity, Vec2 impulse, bool wake)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_ApplyLinearImpulseToCenter(body_id, (b2Vec2){ impulse.x, impulse.y }, wake);
    return true;
}

bool PhysicsWorld_SetEntityAwake(PhysicsWorld* world, struct Entity* entity, bool awake)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_SetAwake(body_id, awake);
    return true;
}

bool PhysicsWorld_IsEntityAwake(PhysicsWorld* world, struct Entity* entity, bool* out_awake)
{
    b2BodyId body_id;

    if (out_awake != NULL) {
        *out_awake = false;
    }
    if (out_awake == NULL || !PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    *out_awake = b2Body_IsAwake(body_id);
    return true;
}

bool PhysicsWorld_GetEntityContactCapacity(PhysicsWorld* world, struct Entity* entity, int* out_contact_capacity)
{
    b2BodyId body_id;

    if (out_contact_capacity != NULL) {
        *out_contact_capacity = 0;
    }
    if (out_contact_capacity == NULL || !PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    *out_contact_capacity = b2Body_GetContactCapacity(body_id);
    return true;
}

void PhysicsWorld_Update(PhysicsWorld* world, struct Scene* scene, float dt_seconds)
{
    double step_started_ms = 0.0;

    if (world == NULL || scene == NULL || !world->has_world)
    {
        return;
    }

    PhysicsWorld_SyncEntities(world, scene);
    if (dt_seconds > 0.0f)
    {
        step_started_ms = PhysicsWorld_NowMs();
        b2World_Step(world->world_id, (float)dt_seconds, (int)world->step_substep_count);
        world->last_box2d_step_wall_ms = PhysicsWorld_NowMs() - step_started_ms;
    }
    scene->last_physics_substeps = dt_seconds > 0.0f ? 1u : 0u;
    PhysicsWorld_SyncBodiesToTransforms(world, scene);
}

void PhysicsWorld_SetTilemap(PhysicsWorld* world,
                               const struct SceneTilemapAdapter* adapter,
                               const void* tilemap,
                               const struct TileRule* tile_rules,
                               size_t tile_rule_count)
{
    if (world == NULL)
    {
        return;
    }

    world->tilemap_adapter = adapter;
    world->tilemap = tilemap;
    world->tile_rules = tile_rules;
    world->tile_rule_count = tile_rule_count;
    PhysicsWorld_RebuildTilemapBodies(world);
}

size_t PhysicsWorld_QueryPoint(PhysicsWorld* world, float x, float y, PhysicsQueryResult* results, size_t capacity)
{
    b2AABB aabb;
    b2QueryFilter filter = b2DefaultQueryFilter();
    PhysicsQueryAccumulator accumulator = {world, results, capacity, 0u, {x, y}};

    if (world == NULL || !world->has_world || results == NULL || capacity == 0)
    {
        return 0;
    }

    aabb.lowerBound = (b2Vec2){x - 0.001f, y - 0.001f};
    aabb.upperBound = (b2Vec2){x + 0.001f, y + 0.001f};
    b2World_OverlapAABB(world->world_id, aabb, filter, PhysicsWorld_OverlapCollector, &accumulator);
    return accumulator.count;
}

size_t PhysicsWorld_QueryRay(PhysicsWorld* world,
                               float x0,
                               float y0,
                               float x1,
                               float y1,
                               PhysicsQueryResult* results,
                               size_t capacity)
{
    b2QueryFilter filter = b2DefaultQueryFilter();
    PhysicsQueryAccumulator accumulator = {world, results, capacity, 0u, {x0, y0}};

    if (world == NULL || !world->has_world || results == NULL || capacity == 0)
    {
        return 0;
    }

    b2World_CastRay(world->world_id,
                    (b2Vec2){x0, y0},
                    (b2Vec2){x1 - x0, y1 - y0},
                    filter,
                    PhysicsWorld_RayCollector,
                    &accumulator);
    return accumulator.count;
}

size_t PhysicsWorld_QueryRadius(PhysicsWorld* world, float x, float y, float radius, PhysicsQueryResult* results, size_t capacity)
{
    b2AABB aabb;
    b2QueryFilter filter = b2DefaultQueryFilter();
    PhysicsQueryAccumulator accumulator = {world, results, capacity, 0u, {x, y}};

    if (world == NULL || !world->has_world || results == NULL || capacity == 0)
    {
        return 0;
    }

    aabb.lowerBound = (b2Vec2){x - radius, y - radius};
    aabb.upperBound = (b2Vec2){x + radius, y + radius};
    b2World_OverlapAABB(world->world_id, aabb, filter, PhysicsWorld_OverlapCollector, &accumulator);
    return accumulator.count;
}

size_t PhysicsWorld_GetContactHits(PhysicsWorld* world, PhysicsContactHit* hits, size_t capacity)
{
    b2ContactEvents contact_events;
    size_t count = 0U;
    int event_index;

    if (world == NULL || !world->has_world || hits == NULL || capacity == 0U) {
        return 0U;
    }

    contact_events = b2World_GetContactEvents(world->world_id);
    for (event_index = 0; event_index < contact_events.hitCount && count < capacity; ++event_index) {
        const b2ContactHitEvent* hit = &contact_events.hitEvents[event_index];
        PhysicsContactHit* out_hit = &hits[count++];
        b2BodyId body_a = b2Shape_GetBody(hit->shapeIdA);
        b2BodyId body_b = b2Shape_GetBody(hit->shapeIdB);

        out_hit->point.x = hit->point.x;
        out_hit->point.y = hit->point.y;
        out_hit->normal.x = hit->normal.x;
        out_hit->normal.y = hit->normal.y;
        out_hit->entity_a = PhysicsWorld_GetEntityForBody(world, body_a);
        out_hit->entity_b = PhysicsWorld_GetEntityForBody(world, body_b);
    }

    return count;
}

static struct Entity* PhysicsWorld_GetEntityForBody(PhysicsWorld* world, b2BodyId body_id)
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
    if (world->has_world)
    {
        b2DestroyWorld(world->world_id);
        world->has_world = false;
    }
    free(world->tile_bodies);
    world->tile_bodies = NULL;
    world->tile_body_count = 0;
    world->tile_body_capacity = 0;
    free(world->tracked_entities);
    free(world);
}
