#include "PhysicsWorld.h"

#include "../Core/TaskSystem.h"
#include "../Scene/Scene.h"
#include "../Scene/Entity.h"
#include "../Scene/Components/ColliderComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Components/TransformComponent.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

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
    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
    if (transform == NULL || rigid_body == NULL || collider == NULL)
    {
        PhysicsWorld_RemoveBodyForEntity(world, entity);
        return b2_nullBodyId;
    }

    if (rigid_body->has_body && b2Body_IsValid(rigid_body->body_id))
    {
        return rigid_body->body_id;
    }

    {
        b2BodyDef body_def = b2DefaultBodyDef();
        b2ShapeDef shape_def = b2DefaultShapeDef();
        b2BodyId body_id;

        body_def.type = RigidBodyComponent_ToBox2DType(rigid_body->body_type);
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
            rigid_body->shape_id = b2CreatePolygonShape(body_id, &shape_def, &box);
        }
        else
        {
            b2Circle circle;
            circle.center = (b2Vec2){0.0f, 0.0f};
            circle.radius = collider->radius;
            rigid_body->shape_id = b2CreateCircleShape(body_id, &shape_def, &circle);
        }

        rigid_body->body_id = body_id;
        rigid_body->has_body = true;
        return body_id;
    }
}

static void PhysicsWorld_SyncEntities(PhysicsWorld* world, struct Scene* scene)
{
    size_t index = 0;

    if (world == NULL || scene == NULL)
    {
        return;
    }

    for (index = 0; index < scene->entity_count; ++index)
    {
        Entity* entity = scene->entities[index];
        TransformComponent* transform = NULL;
        RigidBodyComponent* rigid_body = NULL;
        ColliderComponent* collider = NULL;

        if (entity == NULL || !entity->active)
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
        rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
        collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
        if (transform == NULL || rigid_body == NULL || collider == NULL)
        {
            PhysicsWorld_RemoveBodyForEntity(world, entity);
            continue;
        }

        if (b2Body_IsValid(PhysicsWorld_EnsureEntityBody(world, entity)) && transform->dirty)
        {
            b2Body_SetTransform(rigid_body->body_id, (b2Vec2){transform->x, transform->y}, b2MakeRot(transform->angle_radians));
            transform->dirty = false;
        }
    }
}

static void PhysicsWorld_SyncBodiesToTransforms(PhysicsWorld* world, struct Scene* scene)
{
    size_t index = 0;

    if (world == NULL || scene == NULL)
    {
        return;
    }

    for (index = 0; index < scene->entity_count; ++index)
    {
        Entity* entity = scene->entities[index];
        TransformComponent* transform = NULL;
        RigidBodyComponent* rigid_body = NULL;

        if (entity == NULL || !entity->active)
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
        rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
        if (transform == NULL || rigid_body == NULL || !rigid_body->has_body || !b2Body_IsValid(rigid_body->body_id))
        {
            continue;
        }

        {
            b2Transform body_transform = b2Body_GetTransform(rigid_body->body_id);
            transform->x = body_transform.p.x;
            transform->y = body_transform.p.y;
            transform->angle_radians = b2Rot_GetAngle(body_transform.q);
        }
    }
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
        body_id, shape_id, PhysicsWorld_GetEntityForBody(accumulator->world, body_id), {0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f};
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
        body_id, shape_id, PhysicsWorld_GetEntityForBody(accumulator->world, body_id), point, normal, fraction};
    return 1.0f;
}

void PhysicsWorld_Init(PhysicsWorld* world, const PhysicsWorldConfig* config)
{
    b2WorldDef world_def;
    float default_levels[] = {60.0f, 30.0f, 20.0f, 15.0f, 10.0f};
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
    world->step_substep_count = config != NULL && config->step_substep_count > 0 ? config->step_substep_count : 4u;
    world->adaptive_enabled = config != NULL && config->adaptive_enabled;
    world->downshift_frame_ms = config != NULL && config->downshift_frame_ms > 0.0f ? config->downshift_frame_ms : 33.4f;
    world->upshift_frame_ms = config != NULL && config->upshift_frame_ms > 0.0f ? config->upshift_frame_ms : 20.0f;
    world->tuner_hold_frames = config != NULL && config->tuner_hold_frames > 0 ? config->tuner_hold_frames : 45u;
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

void PhysicsWorld_SetTargetHz(PhysicsWorld* world, float target_hz, const char* reason)
{
    if (world == NULL)
    {
        return;
    }

    world->target_hz = target_hz < 1.0f ? 1.0f : floorf(target_hz);
    world->fixed_dt = 1.0f / world->target_hz;
    world->current_level_index = (uint32_t)PhysicsWorld_FindNearestLevelIndex(world, world->target_hz);
    world->accumulator = fminf(world->accumulator, world->fixed_dt * (float)world->max_substeps);
    if (reason != NULL)
    {
        memset(world->last_tuner_reason, 0, sizeof(world->last_tuner_reason));
        strncpy(world->last_tuner_reason, reason, sizeof(world->last_tuner_reason) - 1);
    }
}

void PhysicsWorld_UpdateAdaptiveBudget(PhysicsWorld* world, float frame_ms)
{
    if (world == NULL || !world->adaptive_enabled || world->adaptive_level_count == 0)
    {
        return;
    }

    if (frame_ms >= world->downshift_frame_ms)
    {
        world->tuner_over_budget_frames += 1u;
        world->tuner_under_budget_frames = 0u;
        if (world->tuner_over_budget_frames >= world->tuner_hold_frames &&
            world->current_level_index + 1u < world->adaptive_level_count)
        {
            world->current_level_index += 1u;
            PhysicsWorld_SetTargetHz(world, world->adaptive_levels[world->current_level_index], "downshift");
            world->tuner_over_budget_frames = 0u;
        }
    }
    else if (frame_ms > 0.0f && frame_ms <= world->upshift_frame_ms)
    {
        world->tuner_under_budget_frames += 1u;
        world->tuner_over_budget_frames = 0u;
        if (world->tuner_under_budget_frames >= world->tuner_hold_frames && world->current_level_index > 0u)
        {
            world->current_level_index -= 1u;
            PhysicsWorld_SetTargetHz(world, world->adaptive_levels[world->current_level_index], "upshift");
            world->tuner_under_budget_frames = 0u;
        }
    }
    else
    {
        world->tuner_over_budget_frames = 0u;
        world->tuner_under_budget_frames = 0u;
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
    if (rigid_body == NULL || !rigid_body->has_body || !b2Body_IsValid(rigid_body->body_id))
    {
        return;
    }

    b2DestroyBody(rigid_body->body_id);
    rigid_body->body_id = b2_nullBodyId;
    rigid_body->shape_id = b2_nullShapeId;
    rigid_body->has_body = false;
}

void PhysicsWorld_ClearEntityBodies(PhysicsWorld* world, struct Scene* scene)
{
    size_t index = 0;

    if (world == NULL || scene == NULL)
    {
        return;
    }

    for (index = 0; index < scene->entity_count; ++index)
    {
        PhysicsWorld_RemoveBodyForEntity(world, scene->entities[index]);
    }
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

    transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
    if (transform == NULL)
    {
        return;
    }

    transform->x = x;
    transform->y = y;

    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
    if (world != NULL && rigid_body != NULL && collider != NULL)
    {
        b2BodyId body_id = rigid_body->has_body ? rigid_body->body_id : PhysicsWorld_EnsureEntityBody(world, entity);
        if (b2Body_IsValid(body_id))
        {
            b2Body_SetTransform(body_id, (b2Vec2){x, y}, b2MakeRot(transform->angle_radians));
            b2Body_SetLinearVelocity(body_id, (b2Vec2){0.0f, 0.0f});
            b2Body_SetAngularVelocity(body_id, 0.0f);
            transform->dirty = false;
            return;
        }
    }

    transform->dirty = true;
}

void PhysicsWorld_Update(PhysicsWorld* world, struct Scene* scene, float dt_seconds)
{
    uint32_t substeps = 0;

    if (world == NULL || scene == NULL || !world->has_world)
    {
        return;
    }

    PhysicsWorld_SyncEntities(world, scene);
    world->accumulator =
        PhysicsWorld_ClampFloat(world->accumulator + (dt_seconds > 0.0f ? dt_seconds : 0.0f), 0.0f, world->fixed_dt * world->max_substeps);

    while (world->accumulator >= world->fixed_dt && substeps < world->max_substeps)
    {
        b2World_Step(world->world_id, world->fixed_dt, (int)world->step_substep_count);
        world->accumulator -= world->fixed_dt;
        substeps += 1u;
    }

    scene->last_physics_substeps = substeps;
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

struct Entity* PhysicsWorld_GetEntityForBody(PhysicsWorld* world, b2BodyId body_id)
{
    (void)world;
    if (!b2Body_IsValid(body_id))
    {
        return NULL;
    }

    return (struct Entity*)b2Body_GetUserData(body_id);
}

void PhysicsWorld_GetSnapshot(const PhysicsWorld* world, PhysicsWorldSnapshot* snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    if (world == NULL)
    {
        return;
    }

    snapshot->body_count = world->has_world ? (uint32_t)b2World_GetAwakeBodyCount(world->world_id) : 0u;
    snapshot->tile_body_count = (uint32_t)world->tile_body_count;
    snapshot->collision_pair_count = world->has_world ? (uint32_t)b2World_GetContactEvents(world->world_id).beginCount : 0u;
    snapshot->physics_hz = world->target_hz;
    snapshot->physics_fixed_dt = world->fixed_dt;
    snapshot->physics_accumulator = world->accumulator;
    snapshot->adaptive_enabled = world->adaptive_enabled;
    snapshot->tuner_level_index = world->current_level_index;
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
    free(world);
}
