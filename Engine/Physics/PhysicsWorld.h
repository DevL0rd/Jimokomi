#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_H

#include "../Core/Geometry.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct TaskSystem;
struct Entity;
struct Scene;
struct RigidBodyComponent;
struct TileRule;
struct SceneTilemapAdapter;

typedef struct PhysicsWorldConfig
{
    float gravity_x;
    float gravity_y;
    float target_hz;
    float sleep_threshold;
    bool continuous_collision_enabled;
    bool has_continuous_collision_setting;
    bool has_sleep_threshold_setting;
    uint32_t max_substeps;
    uint32_t step_substep_count;
    const struct TaskSystem* task_system;
} PhysicsWorldConfig;

typedef struct PhysicsQueryResult
{
    struct Entity* entity;
    Vec2 point;
    Vec2 normal;
    float fraction;
} PhysicsQueryResult;

typedef struct PhysicsWorldSnapshot
{
    uint32_t body_count;
    uint32_t total_body_count;
    uint32_t sleeping_body_count;
    uint32_t moved_body_count;
    uint32_t tile_body_count;
    uint32_t collision_pair_count;
    uint32_t particle_system_count;
    uint32_t particle_group_count;
    uint32_t particle_count;
    uint32_t particle_contact_count;
    uint32_t particle_body_contact_count;
    uint32_t particle_task_count;
    uint32_t particle_task_range_count;
    uint32_t particle_task_range_min;
    uint32_t particle_task_range_max;
    uint32_t particle_spatial_cell_count;
    uint32_t particle_occupied_cell_count;
    uint32_t particle_spatial_proxy_count;
    uint32_t particle_spatial_scatter_count;
    uint32_t particle_contact_candidate_count;
    uint32_t particle_body_shape_candidate_count;
    uint32_t particle_barrier_candidate_count;
    uint32_t particle_reduction_delta_count;
    uint32_t particle_reduction_apply_count;
    uint32_t particle_group_refresh_count;
    uint32_t particle_compaction_move_count;
    uint32_t particle_compaction_remap_count;
    uint32_t particle_byte_count;
    uint32_t particle_scratch_byte_count;
    uint32_t particle_scratch_high_water_byte_count;
    float particle_profile_ms;
    float particle_profile_lifetimes_ms;
    float particle_profile_proxies_ms;
    float particle_profile_spatial_index_ms;
    float particle_profile_contacts_ms;
    float particle_profile_body_contacts_ms;
    float particle_profile_weights_ms;
    float particle_profile_forces_ms;
    float particle_profile_pressure_ms;
    float particle_profile_damping_ms;
    float particle_profile_reduction_ms;
    float particle_profile_collision_ms;
    float particle_profile_groups_ms;
    float particle_profile_barrier_ms;
    float particle_profile_compaction_ms;
    float particle_profile_scratch_ms;
    float particle_profile_events_ms;
    uint64_t physics_step_version;
    float physics_hz;
    float physics_fixed_dt;
    float physics_accumulator;
    uint32_t physics_max_substeps;
    uint32_t physics_step_substeps;
    float corephys_step_wall_ms;
    uint32_t active_entity_count;
    uint32_t dirty_entity_count;
    uint32_t collider_changed_entity_count;
    uint32_t body_create_count;
    uint32_t body_remove_count;
    uint32_t shape_change_count;
    uint32_t task_worker_count;
    uint32_t task_background_thread_count;
    uint32_t corephys_enqueued_task_count;
    uint32_t corephys_inline_task_count;
    uint32_t corephys_main_chunk_count;
    uint32_t corephys_worker_chunk_count;
} PhysicsWorldSnapshot;

typedef struct PhysicsContactHit
{
    Vec2 point;
    Vec2 normal;
    struct Entity* entity_a;
    struct Entity* entity_b;
} PhysicsContactHit;

typedef struct PhysicsWorld PhysicsWorld;

void PhysicsWorld_Init(PhysicsWorld* world, const PhysicsWorldConfig* config);
PhysicsWorld* PhysicsWorld_Create(const PhysicsWorldConfig* config);
void PhysicsWorld_Destroy(PhysicsWorld* world);

void PhysicsWorld_GetStepConfig(const PhysicsWorld* world, float* out_fixed_dt, uint32_t* out_max_substeps);
uint64_t PhysicsWorld_GetStepVersion(const PhysicsWorld* world);
void PhysicsWorld_SetSleepThreshold(PhysicsWorld* world, float sleep_threshold);
void PhysicsWorld_UpdateViewSleepThresholds(
    PhysicsWorld* world,
    struct Entity* const* onscreen_entities,
    size_t onscreen_entity_count,
    float onscreen_sleep_threshold,
    float offscreen_sleep_threshold
);
void PhysicsWorld_RegisterEntity(PhysicsWorld* world, struct Entity* entity);
void PhysicsWorld_UnregisterEntity(PhysicsWorld* world, struct Entity* entity);

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
size_t PhysicsWorld_GetContactHitCount(PhysicsWorld* world);
size_t PhysicsWorld_GetContactHits(PhysicsWorld* world, PhysicsContactHit* hits, size_t capacity);
void PhysicsWorld_GetSnapshot(const PhysicsWorld* world, PhysicsWorldSnapshot* snapshot);

#endif
