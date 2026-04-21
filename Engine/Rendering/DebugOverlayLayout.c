#include "DebugOverlayInternal.h"

#include "../Settings.h"

float debug_title_bar_height(void) {
    return EngineSettings_GetDefaults()->debug_overlay_title_bar_height;
}

float debug_inspector_collapsed_width(void) {
    return EngineSettings_GetDefaults()->debug_overlay_inspector_collapsed_width;
}

bool debug_point_in_rect(float x, float y, Rect rect) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

Rect debug_panel_rect(const DebugPanelState* panel) {
    Rect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (panel != NULL) {
        rect.x = panel->x;
        rect.y = panel->y;
        rect.w = panel->width;
        rect.h = panel->height;
    }
    return rect;
}

Rect debug_title_rect(const DebugPanelState* panel) {
    Rect rect = debug_panel_rect(panel);
    rect.h = debug_title_bar_height();
    return rect;
}

Rect debug_inspector_collapse_rect(const DebugPanelState* panel) {
    Rect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (panel != NULL) {
        rect.x = panel->x;
        rect.y = panel->y;
        rect.w = debug_inspector_collapsed_width();
        rect.h = panel->height;
    }
    return rect;
}

void debug_overlay_ensure_layout(
    DebugOverlay* overlay,
    bool has_selection,
    int viewport_width,
    int viewport_height
) {
    if (overlay == NULL) {
        return;
    }

    if (overlay->layout_initialized &&
        overlay->last_layout_viewport_width == viewport_width &&
        overlay->last_layout_viewport_height == viewport_height &&
        overlay->last_layout_has_selection == has_selection) {
        return;
    }

    if (!overlay->layout_initialized) {
        overlay->dashboard_panel.width = 336.0f;
        overlay->dashboard_panel.height = 548.0f;
        overlay->dashboard_panel.x = 12.0f;
        overlay->dashboard_panel.y = 12.0f;

        overlay->inspector_panel.width = 336.0f;
        overlay->inspector_panel.height = 166.0f;
        overlay->inspector_panel.x = (float)viewport_width - debug_inspector_collapsed_width() - 12.0f;
        overlay->inspector_panel.y = 12.0f;
        overlay->inspector_collapsed = true;
        overlay->layout_initialized = true;
    }

    overlay->dashboard_panel.x = clamp_f(overlay->dashboard_panel.x, 0.0f, (float)viewport_width - overlay->dashboard_panel.width);
    overlay->dashboard_panel.y = clamp_f(overlay->dashboard_panel.y, 0.0f, (float)viewport_height - overlay->dashboard_panel.height);
    overlay->inspector_panel.x = clamp_f(
        overlay->inspector_panel.x,
        0.0f,
        (float)viewport_width - (overlay->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->inspector_panel.width)
    );
    overlay->inspector_panel.y = clamp_f(overlay->inspector_panel.y, 0.0f, (float)viewport_height - overlay->inspector_panel.height);

    if (!has_selection) {
        overlay->inspector_panel.dragging = false;
        overlay->inspector_collapsed = true;
    }

    overlay->last_layout_viewport_width = viewport_width;
    overlay->last_layout_viewport_height = viewport_height;
    overlay->last_layout_has_selection = has_selection;
}
