#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_INTERNAL_H

#include "Renderer.h"

struct Renderer {
    DebugOverlay* debug_overlay;
    Camera camera;
    ResourceManager* resource_manager;
    int view_width;
    int view_height;
    SpriteRenderable* scratch_items;
    size_t scratch_capacity;
    SurfaceDrawInstance* scratch_instances;
    size_t scratch_instance_capacity;
    size_t last_render_item_count;
    size_t last_sprite_draw_count;
    size_t last_visible_item_count;
    size_t last_overlay_draw_count;
    size_t last_procedural_item_count;
    size_t last_baked_surface_count;
    size_t last_instanced_batch_count;
    size_t last_instanced_draw_count;
    double last_sort_ms;
    double last_visibility_ms;
    double last_body_draw_ms;
    double last_overlay_draw_ms;
    double last_instance_draw_ms;
    uint32_t dirty_flags;
    uint64_t last_frame_signature;
    uint64_t last_sort_signature;
    uint64_t last_overlay_signature;
    uint64_t last_instance_batch_signature;
    uint64_t last_backdrop_signature;
    uint64_t last_metadata_signature;
};

#endif
