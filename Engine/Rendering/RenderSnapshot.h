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

void render_world_snapshot_build_frame(const RenderWorldSnapshot* snapshot, RendererFrame* frame);

#endif
