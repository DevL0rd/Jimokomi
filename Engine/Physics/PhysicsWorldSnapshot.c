#include "PhysicsWorldLifecycleInternal.h"
#include "PhysicsWorldStatsInternal.h"
#include "PhysicsWorldTilemapInternal.h"

#include "../Core/Profiling.h"

#include <string.h>

static uint32_t PhysicsWorld_SnapshotCounter(int value)
{
    return value > 0 ? (uint32_t)value : 0U;
}

void PhysicsWorld_GetSnapshot(const PhysicsWorld* world, PhysicsWorldSnapshot* snapshot)
{
    ENGINE_PROFILE_ZONE_BEGIN(snapshot_zone, "PhysicsWorld_GetSnapshot");
    if (snapshot == NULL)
    {
        ENGINE_PROFILE_ZONE_END(snapshot_zone);
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    if (world == NULL)
    {
        ENGINE_PROFILE_ZONE_END(snapshot_zone);
        return;
    }

    snapshot->body_count = (uint32_t)world->stats->awake_body_count;
    snapshot->total_body_count = (uint32_t)world->stats->active_entity_count;
    snapshot->sleeping_body_count = (uint32_t)world->stats->sleeping_body_count;
    snapshot->moved_body_count = (uint32_t)world->stats->moved_body_count;
    snapshot->tile_body_count = (uint32_t)world->tilemap->tile_body_count;
    snapshot->collision_pair_count = world->lifecycle->has_world ? (uint32_t)b2World_GetContactEvents(world->lifecycle->world_id).beginCount : 0u;
    if (world->lifecycle->has_world)
    {
        b2Counters counters = b2World_GetCounters(world->lifecycle->world_id);
        b2Profile profile = b2World_GetProfile(world->lifecycle->world_id);
        snapshot->particle_system_count = PhysicsWorld_SnapshotCounter(counters.particleSystemCount);
        snapshot->particle_group_count = PhysicsWorld_SnapshotCounter(counters.particleGroupCount);
        snapshot->particle_count = PhysicsWorld_SnapshotCounter(counters.particleCount);
        snapshot->particle_contact_count = PhysicsWorld_SnapshotCounter(counters.particleContactCount);
        snapshot->particle_body_contact_count = PhysicsWorld_SnapshotCounter(counters.particleBodyContactCount);
        snapshot->particle_task_count = PhysicsWorld_SnapshotCounter(counters.particleTaskCount);
        snapshot->particle_task_range_count = PhysicsWorld_SnapshotCounter(counters.particleTaskRangeCount);
        snapshot->particle_task_range_min = PhysicsWorld_SnapshotCounter(counters.particleTaskRangeMin);
        snapshot->particle_task_range_max = PhysicsWorld_SnapshotCounter(counters.particleTaskRangeMax);
        snapshot->particle_spatial_cell_count = PhysicsWorld_SnapshotCounter(counters.particleSpatialCellCount);
        snapshot->particle_occupied_cell_count = PhysicsWorld_SnapshotCounter(counters.particleOccupiedCellCount);
        snapshot->particle_spatial_proxy_count = PhysicsWorld_SnapshotCounter(counters.particleSpatialProxyCount);
        snapshot->particle_spatial_scatter_count = PhysicsWorld_SnapshotCounter(counters.particleSpatialScatterCount);
        snapshot->particle_contact_candidate_count = PhysicsWorld_SnapshotCounter(counters.particleContactCandidateCount);
        snapshot->particle_body_shape_candidate_count = PhysicsWorld_SnapshotCounter(counters.particleBodyShapeCandidateCount);
        snapshot->particle_barrier_candidate_count = PhysicsWorld_SnapshotCounter(counters.particleBarrierCandidateCount);
        snapshot->particle_reduction_delta_count = PhysicsWorld_SnapshotCounter(counters.particleReductionDeltaCount);
        snapshot->particle_reduction_apply_count = PhysicsWorld_SnapshotCounter(counters.particleReductionApplyCount);
        snapshot->particle_group_refresh_count = PhysicsWorld_SnapshotCounter(counters.particleGroupRefreshCount);
        snapshot->particle_compaction_move_count = PhysicsWorld_SnapshotCounter(counters.particleCompactionMoveCount);
        snapshot->particle_compaction_remap_count = PhysicsWorld_SnapshotCounter(counters.particleCompactionRemapCount);
        snapshot->particle_byte_count = PhysicsWorld_SnapshotCounter(counters.particleByteCount);
        snapshot->particle_scratch_byte_count = PhysicsWorld_SnapshotCounter(counters.particleScratchByteCount);
        snapshot->particle_scratch_high_water_byte_count =
            PhysicsWorld_SnapshotCounter(counters.particleScratchHighWaterByteCount);
        snapshot->particle_profile_ms = profile.particles;
        snapshot->particle_profile_lifetimes_ms = profile.particleLifetimes;
        snapshot->particle_profile_proxies_ms = profile.particleProxies;
        snapshot->particle_profile_spatial_index_ms =
            profile.particleSpatialIndexBuild + profile.particleSpatialIndexScatter;
        snapshot->particle_profile_contacts_ms =
            profile.particleContacts + profile.particleContactCandidates + profile.particleContactMerge;
        snapshot->particle_profile_body_contacts_ms =
            profile.particleBodyContacts + profile.particleBodyCandidateQuery;
        snapshot->particle_profile_weights_ms = profile.particleWeights;
        snapshot->particle_profile_forces_ms = profile.particleForces;
        snapshot->particle_profile_pressure_ms = profile.particlePressure;
        snapshot->particle_profile_damping_ms = profile.particleDamping;
        snapshot->particle_profile_reduction_ms =
            profile.particleReductionGenerate + profile.particleReductionApply;
        snapshot->particle_profile_collision_ms = profile.particleCollision;
        snapshot->particle_profile_groups_ms = profile.particleGroups + profile.particleGroupRefresh;
        snapshot->particle_profile_barrier_ms = profile.particleBarrier;
        snapshot->particle_profile_compaction_ms =
            profile.particleCompaction + profile.particleCompactionMark + profile.particleCompactionPrefix +
            profile.particleCompactionScatter + profile.particleCompactionRemap + profile.particleZombie;
        snapshot->particle_profile_scratch_ms = profile.particleScratch;
        snapshot->particle_profile_events_ms = profile.particleEvents;
    }
    snapshot->physics_step_version = world->lifecycle->step_version;
    snapshot->physics_hz = world->lifecycle->target_hz;
    snapshot->physics_fixed_dt = world->lifecycle->fixed_dt;
    snapshot->physics_accumulator = (float)world->stats->last_accumulator_seconds;
    snapshot->physics_max_substeps = world->lifecycle->max_substeps;
    snapshot->physics_step_substeps = world->lifecycle->step_substep_count;
    snapshot->corephys_step_wall_ms = (float)world->stats->last_corephys_step_wall_ms;
    snapshot->active_entity_count = (uint32_t)world->stats->active_entity_count;
    snapshot->dirty_entity_count = (uint32_t)world->stats->dirty_entity_count;
    snapshot->collider_changed_entity_count = (uint32_t)world->stats->collider_changed_entity_count;
    snapshot->body_create_count = (uint32_t)world->stats->body_create_count;
    snapshot->body_remove_count = (uint32_t)world->stats->body_remove_count;
    snapshot->shape_change_count = (uint32_t)world->stats->shape_change_count;
    snapshot->task_worker_count = world->stats->task_worker_count;
    snapshot->task_background_thread_count = world->stats->task_background_thread_count;
    snapshot->corephys_enqueued_task_count = world->stats->corephys_enqueued_task_count;
    snapshot->corephys_inline_task_count = world->stats->corephys_inline_task_count;
    snapshot->corephys_main_chunk_count = world->stats->corephys_main_chunk_count;
    snapshot->corephys_worker_chunk_count = world->stats->corephys_worker_chunk_count;
    ENGINE_PROFILE_ZONE_END(snapshot_zone);
}
