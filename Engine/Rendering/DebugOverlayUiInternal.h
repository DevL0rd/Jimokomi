#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_UI_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_UI_INTERNAL_H

#include "DebugOverlayInternal.h"

struct DebugOverlayUiState {
    bool draw_ui;
    bool draw_world_gizmos;
    bool draw_ui_bounds;
    bool layout_initialized;
    bool inspector_collapsed;
    int last_layout_viewport_width;
    int last_layout_viewport_height;
    bool last_layout_has_selection;
    DebugPanelState dashboard_panel;
    DebugPanelState inspector_panel;
    uint32_t hovered_ui_region;
    double last_ui_layout_ms;
    double last_ui_draw_ms;
    double last_ui_composite_ms;
};

#endif
