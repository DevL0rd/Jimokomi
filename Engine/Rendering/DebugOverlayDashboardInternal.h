#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_DASHBOARD_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_DASHBOARD_INTERNAL_H

#include "DebugOverlayInternal.h"

struct DebugOverlayDashboardState {
    RenderBackend* surface_backend;
    Surface* surface;
    int surface_width;
    int surface_height;
    uint64_t last_signature;
    uint64_t last_redraw_at_ms;
    uint32_t redraw_count;
    uint32_t redraw_skip_count;
    double last_redraw_ms;
};

#endif
