#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_INTERNAL_H

#include "PhysicsWorld.h"

#include <box2d/box2d.h>

struct PhysicsWorld
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
    bool adaptive_enabled;
    float adaptive_levels[8];
    size_t adaptive_level_count;
    float adaptive_min_hz;
    float adaptive_max_hz;
    float adaptive_budget_ms;
    float adaptive_recovery_ms;
    float adaptive_blend_alpha;
    uint32_t current_level_index;
    char last_tuner_reason[16];
    const struct SceneTilemapAdapter* tilemap_adapter;
    const void* tilemap;
    const struct TileRule* tile_rules;
    size_t tile_rule_count;
    b2BodyId* tile_bodies;
    size_t tile_body_count;
    size_t tile_body_capacity;
    struct Entity** tracked_entities;
    size_t tracked_entity_count;
    size_t tracked_entity_capacity;
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
};


#endif
