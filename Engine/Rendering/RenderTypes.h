#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERTYPES_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERTYPES_H

#include "Camera.h"
#include "DebugOverlay.h"
#include "RenderCommon.h"
#include "ResourceTypes.h"
#include "Target.h"

typedef struct ProceduralTextureRenderable {
    float x;
    float y;
    float angle_radians;
    float anchor_x;
    float anchor_y;
    int layer;
    bool visible;
    ResourceHandle texture_handle;
    ResourceHandle procedural_texture_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    Color32 tint;
    void* user_data;
} ProceduralTextureRenderable;

typedef struct TriangleRenderable {
    Vec2 a;
    Vec2 b;
    Vec2 c;
    Color32 color;
    int layer;
    bool visible;
} TriangleRenderable;

typedef struct LineRenderable {
    Vec2 a;
    Vec2 b;
    Color32 color;
    int layer;
    bool visible;
} LineRenderable;

typedef struct ProceduralMeshContext {
    uint64_t now_ms;
    float time_seconds;
    float animation_fps;
    uint32_t frame_index;
} ProceduralMeshContext;

typedef bool (*ProceduralMeshAddTriangleFn)(
    void* user_data,
    Vec2 a,
    Vec2 b,
    Vec2 c,
    Color32 color,
    int layer
);
typedef bool (*ProceduralMeshAddLineFn)(
    void* user_data,
    Vec2 a,
    Vec2 b,
    Color32 color,
    int layer
);

typedef struct ProceduralMeshWriter {
    void* user_data;
    ProceduralMeshAddTriangleFn add_triangle;
    ProceduralMeshAddLineFn add_line;
} ProceduralMeshWriter;

typedef bool (*ProceduralMeshBuilder)(
    ProceduralMeshWriter* writer,
    const ProceduralMeshContext* context,
    void* user_data
);

typedef struct Mesh {
    TriangleRenderable* triangles;
    size_t triangle_count;
    size_t triangle_capacity;
    LineRenderable* lines;
    size_t line_count;
    size_t line_capacity;
} Mesh;

typedef struct ProceduralMeshRenderable {
    ResourceHandle procedural_mesh_handle;
    size_t triangle_start;
    size_t triangle_count;
    size_t line_start;
    size_t line_count;
    int layer;
    bool visible;
    void* user_data;
} ProceduralMeshRenderable;

typedef void (*RendererBackdropDrawFn)(Target* target, const Camera* camera, void* user_data);

typedef struct RendererFrame {
    const ProceduralTextureRenderable *procedural_textures;
    size_t procedural_texture_count;
    const ProceduralMeshRenderable* procedural_meshes;
    size_t procedural_mesh_count;
    const TriangleRenderable* triangles;
    size_t triangle_count;
    const LineRenderable* lines;
    size_t line_count;
    uint64_t procedural_texture_frame_signature;
    uint64_t procedural_texture_sort_signature;
    uint64_t procedural_texture_instance_signature;
    bool procedural_texture_signatures_valid;
    bool procedural_textures_sorted_by_layer;
    RendererBackdropDrawFn backdrop_draw;
    void* backdrop_user_data;
    uint64_t backdrop_signature;
    uint64_t metadata_signature;
    bool metadata_dirty;
    DebugGridView debug_grid;
    DebugEntityView *debug_entities;
    size_t debug_entity_count;
    DebugCollisionView *debug_collisions;
    size_t debug_collision_count;
    uint64_t selected_debug_entity_id;
    uint64_t hovered_debug_entity_id;
    bool draw_scene_renderables;
    bool draw_debug;
    uint64_t now_ms;
} RendererFrame;

#endif
