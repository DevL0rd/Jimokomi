#include "DebugOverlayInternal.h"

#include <string.h>

void debug_overlay_get_stats_snapshot(
    const DebugOverlay* overlay,
    DebugOverlayStatsSnapshot* out_snapshot
) {
    if (out_snapshot == NULL) {
        return;
    }
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    if (overlay == NULL) {
        return;
    }

    out_snapshot->visible_entity_count = overlay->last_visible_entity_count;
    out_snapshot->active_collision_count = overlay->last_active_collision_count;
    out_snapshot->dashboard_redraw_count = overlay->dashboard_redraw_count;
    out_snapshot->dashboard_redraw_skip_count = overlay->dashboard_redraw_skip_count;
    out_snapshot->inspector_redraw_count = overlay->inspector_redraw_count;
    out_snapshot->inspector_redraw_skip_count = overlay->inspector_redraw_skip_count;
    out_snapshot->world_redraw_count = overlay->world_redraw_count;
    out_snapshot->world_redraw_skip_count = overlay->world_redraw_skip_count;
    out_snapshot->dashboard_redraw_ms = overlay->last_dashboard_redraw_ms;
    out_snapshot->inspector_redraw_ms = overlay->last_inspector_redraw_ms;
    out_snapshot->world_redraw_ms = overlay->last_world_redraw_ms;
    out_snapshot->ui_layout_ms = overlay->last_ui_layout_ms;
    out_snapshot->ui_draw_ms = overlay->last_ui_draw_ms;
    out_snapshot->ui_composite_ms = overlay->last_ui_composite_ms;
    out_snapshot->surface_memory_bytes =
        ((size_t)overlay->dashboard_surface_width * (size_t)overlay->dashboard_surface_height +
         (size_t)overlay->inspector_surface_width * (size_t)overlay->inspector_surface_height +
         (size_t)overlay->world_surface_width * (size_t)overlay->world_surface_height) * 4U;
    out_snapshot->draw_ui = overlay->draw_ui;
}

void debug_overlay_toggle(DebugOverlay *overlay) {
    if (overlay != NULL) {
        overlay->draw_ui = !overlay->draw_ui;
        overlay->dashboard_panel.dragging = false;
        overlay->inspector_panel.dragging = false;
        overlay->hovered_ui_region = 0U;
    }
}

void debug_overlay_toggle_world_gizmos(DebugOverlay* overlay) {
    if (overlay != NULL) {
        overlay->draw_world_gizmos = !overlay->draw_world_gizmos;
    }
}

bool debug_overlay_is_ui_visible(const DebugOverlay* overlay) {
    return overlay != NULL && overlay->draw_ui;
}

bool debug_overlay_is_world_gizmos_visible(const DebugOverlay* overlay) {
    return overlay != NULL && overlay->draw_world_gizmos;
}
