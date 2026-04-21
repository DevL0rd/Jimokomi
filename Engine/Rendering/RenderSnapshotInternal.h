#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H

#include "RenderSnapshot.h"

struct RenderWorldSnapshot {
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
};

struct RenderSnapshotBuffer {
    RenderWorldSnapshot world;
    RenderStatsSnapshot stats;
    uint64_t sequence;
    uint64_t published_at_ms;
};

bool render_world_snapshot_reserve_items(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_debug_collisions(RenderWorldSnapshot* snapshot, size_t required_capacity);
void render_world_snapshot_reset(RenderWorldSnapshot* snapshot);
void render_world_snapshot_build_frame(const RenderWorldSnapshot* snapshot, RendererFrame* frame);

#endif
