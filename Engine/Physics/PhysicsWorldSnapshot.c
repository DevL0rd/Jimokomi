#include "PhysicsWorldInternal.h"

#include <string.h>

void PhysicsWorld_GetSnapshot(const PhysicsWorld* world, PhysicsWorldSnapshot* snapshot)
{
    b2Profile profile = { 0 };

    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    if (world == NULL)
    {
        return;
    }

    if (world->has_world)
    {
        profile = b2World_GetProfile(world->world_id);
    }

    snapshot->body_count = (uint32_t)world->awake_body_count;
    snapshot->total_body_count = (uint32_t)world->active_entity_count;
    snapshot->sleeping_body_count = (uint32_t)world->sleeping_body_count;
    snapshot->moved_body_count = (uint32_t)world->moved_body_count;
    snapshot->tile_body_count = (uint32_t)world->tile_body_count;
    snapshot->collision_pair_count = world->has_world ? (uint32_t)b2World_GetContactEvents(world->world_id).beginCount : 0u;
    snapshot->physics_hz = world->target_hz;
    snapshot->physics_fixed_dt = world->fixed_dt;
    snapshot->physics_accumulator = 0.0f;
    snapshot->physics_max_substeps = world->max_substeps;
    snapshot->physics_step_substeps = world->step_substep_count;
    snapshot->profile_step_ms = profile.step;
    snapshot->profile_pairs_ms = profile.pairs;
    snapshot->profile_collide_ms = profile.collide;
    snapshot->profile_solve_ms = profile.solve;
    snapshot->box2d_step_wall_ms = (float)world->last_box2d_step_wall_ms;
    snapshot->active_entity_count = (uint32_t)world->active_entity_count;
    snapshot->dirty_entity_count = (uint32_t)world->dirty_entity_count;
    snapshot->collider_changed_entity_count = (uint32_t)world->collider_changed_entity_count;
    snapshot->body_create_count = (uint32_t)world->body_create_count;
    snapshot->body_remove_count = (uint32_t)world->body_remove_count;
    snapshot->shape_change_count = (uint32_t)world->shape_change_count;
}
