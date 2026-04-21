#include "PhysicsWorldLifecycleInternal.h"
#include "PhysicsWorldStatsInternal.h"
#include "PhysicsWorldTilemapInternal.h"

#include <string.h>

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

    snapshot->body_count = (uint32_t)world->stats->awake_body_count;
    snapshot->total_body_count = (uint32_t)world->stats->active_entity_count;
    snapshot->sleeping_body_count = (uint32_t)world->stats->sleeping_body_count;
    snapshot->moved_body_count = (uint32_t)world->stats->moved_body_count;
    snapshot->tile_body_count = (uint32_t)world->tilemap->tile_body_count;
    snapshot->collision_pair_count = world->lifecycle->has_world ? (uint32_t)b2World_GetContactEvents(world->lifecycle->world_id).beginCount : 0u;
    snapshot->physics_hz = world->lifecycle->target_hz;
    snapshot->physics_fixed_dt = world->lifecycle->fixed_dt;
    snapshot->physics_accumulator = 0.0f;
    snapshot->physics_max_substeps = world->lifecycle->max_substeps;
    snapshot->physics_step_substeps = world->lifecycle->step_substep_count;
    snapshot->box2d_step_wall_ms = (float)world->stats->last_box2d_step_wall_ms;
    snapshot->active_entity_count = (uint32_t)world->stats->active_entity_count;
    snapshot->dirty_entity_count = (uint32_t)world->stats->dirty_entity_count;
    snapshot->collider_changed_entity_count = (uint32_t)world->stats->collider_changed_entity_count;
    snapshot->body_create_count = (uint32_t)world->stats->body_create_count;
    snapshot->body_remove_count = (uint32_t)world->stats->body_remove_count;
    snapshot->shape_change_count = (uint32_t)world->stats->shape_change_count;
}
