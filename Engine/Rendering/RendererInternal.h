#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_INTERNAL_H

#include "Renderer.h"

struct Renderer {
    DebugOverlay* debug_overlay;
    Camera camera;
    ResourceManager* resource_manager;
    int view_width;
    int view_height;
    float prebake_target_fps;
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

typedef struct RendererPreparedSurfaceDraw {
    const Surface* surface;
    SurfaceDrawInstance instance;
    bool valid;
} RendererPreparedSurfaceDraw;

typedef struct RendererSurfaceBatch {
    const Surface* surface;
    Color32 tint;
    size_t count;
} RendererSurfaceBatch;

double renderer_now_ms(void);
uint64_t renderer_compute_overlay_signature(const RendererFrame* frame);
void renderer_compute_frame_signatures(
    const RendererFrame* frame,
    uint64_t* out_frame_signature,
    uint64_t* out_sort_signature,
    uint64_t* out_instance_signature,
    bool* out_items_sorted_by_layer
);
bool renderer_is_item_visible(const Renderer* renderer, const SpriteRenderable* item);
bool renderer_reserve_instances(Renderer* renderer, size_t required_capacity);
bool renderer_prepare_batched_surface_draw(
    Renderer* renderer,
    const SpriteRenderable* item,
    uint64_t now_ms,
    RendererPreparedSurfaceDraw* prepared
);
void renderer_flush_surface_batch(
    Renderer* renderer,
    RenderBackend* backend,
    RendererSurfaceBatch* batch
);

#endif
