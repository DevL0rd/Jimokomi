#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_INTERNAL_H

#include "Renderer.h"

typedef struct RendererLifecycleState RendererLifecycleState;
typedef struct RendererScratchState RendererScratchState;
typedef struct RendererStatsState RendererStatsState;
typedef struct RendererSignatureState RendererSignatureState;

struct Renderer {
    RendererLifecycleState* lifecycle;
    RendererScratchState* scratch;
    RendererStatsState* stats;
    RendererSignatureState* signatures;
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
