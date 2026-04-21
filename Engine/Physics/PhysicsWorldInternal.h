#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_INTERNAL_H

#include "PhysicsWorld.h"
#include "PhysicsHandles.h"

#include <box2d/box2d.h>

typedef struct PhysicsWorldLifecycleState {
    b2WorldId world_id;
    bool has_world;
    float gravity_x;
    float gravity_y;
    float target_hz;
    float fixed_dt;
    uint32_t max_substeps;
    uint32_t base_step_substep_count;
    uint32_t step_substep_count;
} PhysicsWorldLifecycleState;

typedef struct PhysicsWorldTilemapState {
    const struct SceneTilemapAdapter* tilemap_adapter;
    const void* tilemap;
    const struct TileRule* tile_rules;
    size_t tile_rule_count;
    b2BodyId* tile_bodies;
    size_t tile_body_count;
    size_t tile_body_capacity;
} PhysicsWorldTilemapState;

typedef struct PhysicsWorldEntityState {
    struct Entity** tracked_entities;
    size_t tracked_entity_count;
    size_t tracked_entity_capacity;
} PhysicsWorldEntityState;

typedef struct PhysicsWorldStatsState {
    size_t active_entity_count;
    size_t dirty_entity_count;
    size_t collider_changed_entity_count;
    size_t body_create_count;
    size_t body_remove_count;
    size_t shape_change_count;
    size_t awake_body_count;
    size_t sleeping_body_count;
    size_t moved_body_count;
    double last_box2d_step_wall_ms;
} PhysicsWorldStatsState;

struct PhysicsWorld
{
    PhysicsWorldLifecycleState lifecycle;
    PhysicsWorldTilemapState tilemap;
    PhysicsWorldEntityState entities;
    PhysicsWorldStatsState stats;
};

b2BodyId PhysicsWorld_LoadBodyHandle(PhysicsBodyHandle handle);
PhysicsBodyHandle PhysicsWorld_StoreBodyHandle(b2BodyId body_id);
PhysicsShapeHandle PhysicsWorld_StoreShapeHandle(b2ShapeId shape_id);
b2BodyId PhysicsWorld_EnsureEntityBody(PhysicsWorld* world, struct Entity* entity);
bool PhysicsWorld_GetEntityBody(
    PhysicsWorld* world,
    struct Entity* entity,
    bool create_if_missing,
    b2BodyId* out_body_id
);
struct Entity* PhysicsWorld_GetEntityForBody(PhysicsWorld* world, b2BodyId body_id);
void PhysicsWorld_ClearTilemapBodies(PhysicsWorld* world);

#endif
