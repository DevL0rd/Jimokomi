#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H

#include "RenderSnapshot.h"

struct RenderWorldSnapshot {
    ProceduralTextureRenderable* procedural_textures;
    size_t procedural_texture_capacity;
    size_t procedural_texture_count;
    ProceduralMeshRenderable* procedural_meshes;
    size_t procedural_mesh_capacity;
    size_t procedural_mesh_count;
    TriangleRenderable* triangles;
    size_t triangle_capacity;
    size_t triangle_count;
    LineRenderable* lines;
    size_t line_capacity;
    size_t line_count;
    uint64_t procedural_texture_frame_signature;
    uint64_t procedural_texture_sort_signature;
    uint64_t procedural_texture_instance_signature;
    bool procedural_texture_signatures_valid;
    bool procedural_textures_sorted_by_layer;
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
    bool draw_scene_renderables;
    bool draw_debug_world;
    uint64_t now_ms;
};

struct RenderSnapshotBuffer {
    RenderWorldSnapshot world;
    RenderStatsSnapshot stats;
    uint64_t sequence;
    uint64_t published_at_ms;
};

bool render_world_snapshot_reserve_procedural_textures(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_procedural_meshes(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_triangles(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_lines(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_debug_collisions(RenderWorldSnapshot* snapshot, size_t required_capacity);
void render_world_snapshot_reset(RenderWorldSnapshot* snapshot);
void render_world_snapshot_build_frame(const RenderWorldSnapshot* snapshot, RendererFrame* frame);

#endif
