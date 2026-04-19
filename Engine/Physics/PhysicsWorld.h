#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_H

#include <box2d/box2d.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct TaskSystem;
struct Entity;
struct Scene;
struct TileRule;
struct SceneTilemapAdapter;

typedef struct PhysicsWorldConfig
{
    float gravity_x;
    float gravity_y;
    float target_hz;
    bool adaptive_enabled;
    const float* adaptive_levels;
    size_t adaptive_level_count;
    float downshift_frame_ms;
    float upshift_frame_ms;
    uint32_t tuner_hold_frames;
    uint32_t max_substeps;
    uint32_t step_substep_count;
    const struct TaskSystem* task_system;
} PhysicsWorldConfig;

typedef struct PhysicsQueryResult
{
    b2BodyId body_id;
    b2ShapeId shape_id;
    struct Entity* entity;
    b2Vec2 point;
    b2Vec2 normal;
    float fraction;
} PhysicsQueryResult;

typedef struct PhysicsWorldSnapshot
{
    uint32_t body_count;
    uint32_t tile_body_count;
    uint32_t collision_pair_count;
    float physics_hz;
    float physics_fixed_dt;
    float physics_accumulator;
    uint32_t physics_step_substeps;
    bool adaptive_enabled;
    uint32_t tuner_level_index;
} PhysicsWorldSnapshot;

typedef struct PhysicsWorld
{
    b2WorldId world_id;
    bool has_world;
    float gravity_x;
    float gravity_y;
    float target_hz;
    float fixed_dt;
    uint32_t max_substeps;
    uint32_t base_step_substep_count;
    uint32_t step_substep_count;
    float accumulator;
    bool adaptive_enabled;
    float adaptive_levels[8];
    size_t adaptive_level_count;
    uint32_t current_level_index;
    float downshift_frame_ms;
    float upshift_frame_ms;
    uint32_t tuner_hold_frames;
    uint32_t tuner_over_budget_frames;
    uint32_t tuner_under_budget_frames;
    char last_tuner_reason[16];
    const struct SceneTilemapAdapter* tilemap_adapter;
    const void* tilemap;
    const struct TileRule* tile_rules;
    size_t tile_rule_count;
    b2BodyId* tile_bodies;
    size_t tile_body_count;
    size_t tile_body_capacity;
} PhysicsWorld;

void PhysicsWorld_Init(PhysicsWorld* world, const PhysicsWorldConfig* config);
PhysicsWorld* PhysicsWorld_Create(const PhysicsWorldConfig* config);
void PhysicsWorld_Destroy(PhysicsWorld* world);

void PhysicsWorld_SetTargetHz(PhysicsWorld* world, float target_hz, const char* reason);
void PhysicsWorld_UpdateAdaptiveBudget(PhysicsWorld* world, float frame_ms);

void PhysicsWorld_RemoveBodyForEntity(PhysicsWorld* world, struct Entity* entity);
void PhysicsWorld_ClearEntityBodies(PhysicsWorld* world, struct Scene* scene);
void PhysicsWorld_SetEntityPosition(PhysicsWorld* world, struct Entity* entity, float x, float y);

void PhysicsWorld_Update(PhysicsWorld* world, struct Scene* scene, float dt_seconds);
void PhysicsWorld_SetTilemap(PhysicsWorld* world,
                               const struct SceneTilemapAdapter* adapter,
                               const void* tilemap,
                               const struct TileRule* tile_rules,
                               size_t tile_rule_count);

size_t PhysicsWorld_QueryPoint(PhysicsWorld* world, float x, float y, PhysicsQueryResult* results, size_t capacity);
size_t PhysicsWorld_QueryRay(PhysicsWorld* world,
                               float x0,
                               float y0,
                               float x1,
                               float y1,
                               PhysicsQueryResult* results,
                               size_t capacity);
size_t PhysicsWorld_QueryRadius(PhysicsWorld* world, float x, float y, float radius, PhysicsQueryResult* results, size_t capacity);

struct Entity* PhysicsWorld_GetEntityForBody(PhysicsWorld* world, b2BodyId body_id);
void PhysicsWorld_GetSnapshot(const PhysicsWorld* world, PhysicsWorldSnapshot* snapshot);

#endif
