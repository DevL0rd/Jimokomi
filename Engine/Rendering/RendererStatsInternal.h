#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_STATS_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_STATS_INTERNAL_H

#include "RendererInternal.h"

struct RendererStatsState {
    size_t last_procedural_texture_count;
    size_t last_procedural_texture_draw_count;
    size_t last_visible_procedural_texture_count;
    size_t last_overlay_draw_count;
    size_t last_procedural_texture_item_count;
    size_t last_baked_texture_count;
    size_t last_instanced_batch_count;
    size_t last_instanced_draw_count;
    double last_sort_ms;
    double last_visibility_ms;
    double last_body_draw_ms;
    double last_overlay_draw_ms;
    double last_instance_draw_ms;
};

#endif
