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

typedef struct RendererPreparedTextureDraw {
    const Texture* texture;
    TextureDrawInstance instance;
    bool valid;
} RendererPreparedTextureDraw;

typedef struct RendererTextureBatch {
    const Texture* texture;
    Color32 tint;
    size_t count;
} RendererTextureBatch;

double renderer_now_ms(void);
uint64_t renderer_compute_overlay_signature(const RendererFrame* frame);
void renderer_compute_frame_signatures(
    const RendererFrame* frame,
    uint64_t* out_frame_signature,
    uint64_t* out_sort_signature,
    uint64_t* out_instance_signature,
    bool* out_material_renderables_sorted_by_layer
);
bool renderer_is_material_renderable_visible(const Renderer* renderer, const MaterialRenderable* renderable);
bool renderer_reserve_instances(Renderer* renderer, size_t required_capacity);
bool renderer_prepare_batched_material_draw(
    Renderer* renderer,
    const MaterialRenderable* renderable,
    uint64_t now_ms,
    RendererPreparedTextureDraw* prepared
);
void renderer_flush_texture_batch(
    Renderer* renderer,
    RenderBackend* backend,
    RendererTextureBatch* batch
);

#endif
