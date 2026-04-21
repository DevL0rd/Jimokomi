#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_LIFECYCLE_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_LIFECYCLE_INTERNAL_H

#include "PhysicsWorldInternal.h"

struct PhysicsWorldLifecycleState {
    b2WorldId world_id;
    const struct TaskSystem* task_system;
    bool has_world;
    float gravity_x;
    float gravity_y;
    float target_hz;
    float min_hz;
    float max_hz;
    float frame_budget_hz;
    float fixed_dt;
    uint32_t max_substeps;
    uint32_t base_step_substep_count;
    uint32_t step_substep_count;
};

#endif
