#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_DASHBOARD_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_DASHBOARD_INTERNAL_H

#include "DebugOverlayInternal.h"

struct DebugOverlayDashboardState {
    RenderBackend* texture_backend;
    Texture* texture;
    int texture_width;
    int texture_height;
    uint64_t last_signature;
    uint64_t last_redraw_at_ms;
    uint32_t redraw_count;
    uint32_t redraw_skip_count;
    double last_redraw_ms;
};

#endif
