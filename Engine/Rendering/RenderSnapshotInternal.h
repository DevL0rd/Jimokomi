#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H

#include "RenderSnapshot.h"
#include "ParticleVisualResources.h"

struct RenderWorldSnapshot {
    MaterialRenderable* material_renderables;
    size_t material_renderable_capacity;
    size_t material_renderable_count;
    ProceduralMeshRenderable* procedural_meshes;
    size_t procedural_mesh_capacity;
    size_t procedural_mesh_count;
    TriangleRenderable* triangles;
    size_t triangle_capacity;
    size_t triangle_count;
    LineRenderable* lines;
    size_t line_capacity;
    size_t line_count;
    ParticleSurfaceMeshBuildInput* particle_surface_mesh_inputs;
    size_t particle_surface_mesh_input_capacity;
    size_t particle_surface_mesh_input_count;
    PhysicsParticleRenderData* particle_surface_mesh_particles;
    size_t particle_surface_mesh_particle_capacity;
    size_t particle_surface_mesh_particle_count;
    uint64_t material_frame_signature;
    uint64_t material_sort_signature;
    uint64_t material_instance_signature;
    bool material_signatures_valid;
    bool material_renderables_sorted_by_layer;
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

bool render_world_snapshot_reserve_material_renderables(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_procedural_meshes(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_triangles(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_lines(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_particle_surface_mesh_inputs(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_particle_surface_mesh_particles(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_debug_collisions(RenderWorldSnapshot* snapshot, size_t required_capacity);
void render_world_snapshot_reset(RenderWorldSnapshot* snapshot);
void render_world_snapshot_build_frame(const RenderWorldSnapshot* snapshot, RendererFrame* frame);

#endif
