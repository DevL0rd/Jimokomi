#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_STATS_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_STATS_INTERNAL_H

#include "PhysicsWorldInternal.h"

struct PhysicsWorldStatsState {
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
    double last_accumulator_seconds;
};

#endif
