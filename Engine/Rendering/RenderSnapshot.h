#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOT_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOT_H

#include "RenderTypes.h"
#include "../Core/Stats.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct RenderSnapshotExchange RenderSnapshotExchange;

typedef struct RenderSnapshotRenderStats {
    float render_alpha;
} RenderSnapshotRenderStats;

typedef struct RenderSnapshotSimStats {
    float input_ms;
    float game_update_ms;
    float fixed_step_wall_ms;
    float drag_ms;
    float snapshot_acquire_ms;
    float snapshot_build_ms;
} RenderSnapshotSimStats;

typedef struct RenderSnapshotPhysicsStats {
    uint32_t physics_substeps;
    float physics_hz;
    uint32_t physics_step_substeps;
    uint32_t physics_active_entities;
    uint32_t physics_dirty_entities;
    uint32_t physics_collider_changed_entities;
    uint32_t physics_body_create_queue;
    uint32_t physics_body_remove_queue;
    uint32_t physics_shape_change_queue;
} RenderSnapshotPhysicsStats;

typedef struct RenderSnapshotResourceStats {
    size_t pending_bakes;
} RenderSnapshotResourceStats;

typedef struct RenderSnapshotCameraStats {
    float camera_x;
    float camera_y;
} RenderSnapshotCameraStats;

typedef struct RenderStatsSnapshot {
    DebugOverlaySnapshot overlay;
    EngineStatsSnapshot engine_stats;
    RenderSnapshotRenderStats render;
    RenderSnapshotSimStats sim;
    RenderSnapshotPhysicsStats physics;
    RenderSnapshotResourceStats resources;
    RenderSnapshotCameraStats camera;
} RenderStatsSnapshot;

typedef struct PickTargetView {
    uint64_t id;
    float x;
    float y;
    float radius;
    float extent_x;
    float extent_y;
    bool is_circle;
} PickTargetView;

typedef struct RenderWorldSnapshot RenderWorldSnapshot;
typedef struct RenderSnapshotBuffer RenderSnapshotBuffer;

void render_snapshot_buffer_build_frame(const RenderSnapshotBuffer* buffer, RendererFrame* frame);
const RenderStatsSnapshot* render_snapshot_buffer_get_stats(const RenderSnapshotBuffer* buffer);
uint64_t render_snapshot_buffer_get_sequence(const RenderSnapshotBuffer* buffer);
uint64_t render_snapshot_buffer_get_published_at_ms(const RenderSnapshotBuffer* buffer);
uint64_t render_snapshot_buffer_get_now_ms(const RenderSnapshotBuffer* buffer);
const DebugEntityView* render_snapshot_buffer_get_selected_entity(const RenderSnapshotBuffer* buffer);
uint64_t render_snapshot_buffer_pick_entity(const RenderSnapshotBuffer* buffer, const Camera* camera, Vec2 screen_point);
void render_snapshot_buffer_set_sim_timings(
    RenderSnapshotBuffer* buffer,
    float update_ms,
    float sim_ms,
    float input_ms,
    float game_update_ms,
    float fixed_step_wall_ms,
    float drag_ms,
    float snapshot_acquire_ms,
    float snapshot_build_ms
);

#endif
