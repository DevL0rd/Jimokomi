#include "DebugOverlayInternal.h"

#include "DebugOverlayDashboardInternal.h"
#include "DebugOverlayInspectorInternal.h"
#include "DebugOverlayUiInternal.h"
#include "DebugOverlayWorldInternal.h"
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

    out_snapshot->visible_entity_count = overlay->world->last_visible_entity_count;
    out_snapshot->active_collision_count = overlay->world->last_active_collision_count;
    out_snapshot->dashboard_redraw_count = overlay->dashboard->redraw_count;
    out_snapshot->dashboard_redraw_skip_count = overlay->dashboard->redraw_skip_count;
    out_snapshot->inspector_redraw_count = overlay->inspector->redraw_count;
    out_snapshot->inspector_redraw_skip_count = overlay->inspector->redraw_skip_count;
    out_snapshot->world_redraw_count = overlay->world->redraw_count;
    out_snapshot->world_redraw_skip_count = overlay->world->redraw_skip_count;
    out_snapshot->dashboard_redraw_ms = overlay->dashboard->last_redraw_ms;
    out_snapshot->inspector_redraw_ms = overlay->inspector->last_redraw_ms;
    out_snapshot->world_redraw_ms = overlay->world->last_redraw_ms;
    out_snapshot->ui_layout_ms = overlay->ui->last_ui_layout_ms;
    out_snapshot->ui_draw_ms = overlay->ui->last_ui_draw_ms;
    out_snapshot->ui_composite_ms = overlay->ui->last_ui_composite_ms;
    out_snapshot->surface_memory_bytes =
        ((size_t)overlay->dashboard->surface_width * (size_t)overlay->dashboard->surface_height +
         (size_t)overlay->inspector->surface_width * (size_t)overlay->inspector->surface_height +
         (size_t)overlay->world->surface_width * (size_t)overlay->world->surface_height) * 4U;
    out_snapshot->draw_ui = overlay->ui->draw_ui;
}

void debug_overlay_toggle(DebugOverlay *overlay) {
    if (overlay != NULL) {
        overlay->ui->draw_ui = !overlay->ui->draw_ui;
        overlay->ui->dashboard_panel.dragging = false;
        overlay->ui->inspector_panel.dragging = false;
        overlay->ui->hovered_ui_region = 0U;
    }
}

void debug_overlay_toggle_world_gizmos(DebugOverlay* overlay) {
    if (overlay != NULL) {
        overlay->ui->draw_world_gizmos = !overlay->ui->draw_world_gizmos;
    }
}

bool debug_overlay_is_ui_visible(const DebugOverlay* overlay) {
    return overlay != NULL && overlay->ui->draw_ui;
}

bool debug_overlay_is_world_gizmos_visible(const DebugOverlay* overlay) {
    return overlay != NULL && overlay->ui->draw_world_gizmos;
}
