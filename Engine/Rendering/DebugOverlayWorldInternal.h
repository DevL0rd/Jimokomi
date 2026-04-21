#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_WORLD_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_WORLD_INTERNAL_H

#include "DebugOverlayInternal.h"

struct DebugOverlayWorldState {
    RenderBackend* surface_backend;
    Surface* surface;
    int surface_width;
    int surface_height;
    uint64_t last_signature;
    size_t last_visible_entity_count;
    size_t last_active_collision_count;
    uint32_t redraw_count;
    uint32_t redraw_skip_count;
    double last_redraw_ms;
};

#endif
