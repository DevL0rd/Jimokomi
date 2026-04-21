#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOT_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOT_H

#include "RenderTypes.h"
#include "../Core/Stats.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct RenderStatsSnapshot {
    DebugOverlaySnapshot overlay;
    EngineStatsSnapshot engine_stats;
    float render_alpha;
    float cull_grid_ms;
    float sim_input_ms;
    float sim_spawn_ms;
    float sim_fixed_step_wall_ms;
    float sim_drag_ms;
    float sim_snapshot_acquire_ms;
    float sim_snapshot_build_ms;
    uint32_t cull_grid_candidates;
    int32_t cull_grid_span_min_x;
    int32_t cull_grid_span_max_x;
    int32_t cull_grid_span_min_y;
    int32_t cull_grid_span_max_y;
    uint32_t cull_grid_span_cells;
    uint32_t physics_substeps;
    float physics_hz;
    uint32_t physics_step_substeps;
    uint32_t physics_active_entities;
    uint32_t physics_dirty_entities;
    uint32_t physics_collider_changed_entities;
    uint32_t physics_body_create_queue;
    uint32_t physics_body_remove_queue;
    uint32_t physics_shape_change_queue;
    size_t pending_bakes;
    uint64_t resource_commands_enqueued;
    uint64_t resource_commands_dropped;
    float camera_x;
    float camera_y;
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

typedef struct RenderWorldSnapshot {
    SpriteRenderable* items;
    size_t item_capacity;
    size_t item_count;
    uint64_t item_frame_signature;
    uint64_t item_sort_signature;
    uint64_t item_instance_signature;
    bool item_signatures_valid;
    bool items_sorted_by_layer;
    RendererBackdropDrawFn backdrop_draw;
    void* backdrop_user_data;
    DebugGridView debug_grid;
    uint64_t backdrop_signature;
    uint64_t metadata_signature;
    bool metadata_dirty;
    DebugEntityView* debug_entities;
    size_t debug_entity_capacity;
    size_t debug_entity_count;
    DebugCollisionView* debug_collisions;
    size_t debug_collision_capacity;
    size_t debug_collision_count;
    PickTargetView* pick_targets;
    size_t pick_target_capacity;
    size_t pick_target_count;
    Camera camera;
    DebugEntityView selected_entity;
    bool has_selected_entity;
    DebugEntityView hovered_entity;
    bool has_hovered_entity;
    bool draw_sprites;
    bool draw_debug_world;
    uint64_t now_ms;
} RenderWorldSnapshot;

typedef struct RenderSnapshotBuffer {
    RenderWorldSnapshot world;
    RenderStatsSnapshot stats;
    uint64_t sequence;
    uint64_t published_at_ms;
} RenderSnapshotBuffer;

typedef struct RenderSnapshotExchange {
    RenderSnapshotBuffer buffers[3];
    size_t write_index;
    size_t item_capacity;
    size_t debug_entity_capacity;
    size_t debug_collision_capacity;
    atomic_size_t published_index;
    atomic_uint_fast64_t published_sequence;
    atomic_uint reader_counts[3];
} RenderSnapshotExchange;

bool render_snapshot_exchange_init(
    RenderSnapshotExchange* exchange,
    size_t item_capacity,
    size_t debug_entity_capacity,
    size_t debug_collision_capacity
);
void render_snapshot_exchange_dispose(RenderSnapshotExchange* exchange);
RenderSnapshotBuffer* render_snapshot_exchange_begin_write(RenderSnapshotExchange* exchange);
void render_snapshot_exchange_publish(RenderSnapshotExchange* exchange, RenderSnapshotBuffer* buffer);
uint64_t render_snapshot_exchange_get_published_sequence(const RenderSnapshotExchange* exchange);
const RenderSnapshotBuffer* render_snapshot_exchange_acquire_published(RenderSnapshotExchange* exchange);
const RenderSnapshotBuffer* render_snapshot_exchange_acquire_if_new(
    RenderSnapshotExchange* exchange,
    uint64_t last_sequence
);
void render_snapshot_exchange_release_published(RenderSnapshotExchange* exchange, const RenderSnapshotBuffer* buffer);
void render_world_snapshot_reset(RenderWorldSnapshot* snapshot);
void render_world_snapshot_build_frame(const RenderWorldSnapshot* snapshot, RendererFrame* frame);

#endif
