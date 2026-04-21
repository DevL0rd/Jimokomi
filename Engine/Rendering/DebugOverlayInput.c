#include "DebugOverlayInternal.h"

static bool debug_handle_panel_drag(
    DebugPanelState* panel,
    const EngineInput* input,
    bool pressed,
    bool released
) {
    Rect title_rect;
    if (panel == NULL || input == NULL) {
        return false;
    }

    title_rect = debug_title_rect(panel);
    if (pressed && debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, title_rect)) {
        panel->dragging = true;
        panel->drag_offset_x = (float)input->mouse_x - panel->x;
        panel->drag_offset_y = (float)input->mouse_y - panel->y;
        return true;
    }

    if (panel->dragging) {
        if (released || !EngineInput_is_mouse_down(input, 1U)) {
            panel->dragging = false;
        } else {
            panel->x = (float)input->mouse_x - panel->drag_offset_x;
            panel->y = (float)input->mouse_y - panel->drag_offset_y;
        }
        return true;
    }

    return false;
}

void debug_overlay_handle_input(
    DebugOverlay* overlay,
    const EngineInput* input,
    bool has_selection,
    int viewport_width,
    int viewport_height
) {
    bool pressed;
    bool released;
    bool handled;

    if (overlay == NULL || input == NULL || !overlay->draw_ui) {
        return;
    }

    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    pressed = EngineInput_was_mouse_pressed(input, 1U);
    released = EngineInput_was_mouse_released(input, 1U);

    handled = debug_handle_panel_drag(&overlay->dashboard_panel, input, pressed, released);
    overlay->hovered_ui_region = DEBUG_UI_HOVER_NONE;
    if (debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, debug_title_rect(&overlay->dashboard_panel))) {
        overlay->hovered_ui_region = DEBUG_UI_HOVER_DASHBOARD;
    }
    if (!handled && has_selection) {
        if (debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, overlay->inspector_collapsed
                ? debug_inspector_collapse_rect(&overlay->inspector_panel)
                : debug_title_rect(&overlay->inspector_panel))) {
            overlay->hovered_ui_region = DEBUG_UI_HOVER_INSPECTOR;
        }
        if (pressed && debug_point_in_rect((float)input->mouse_x, (float)input->mouse_y, overlay->inspector_collapsed
                ? debug_inspector_collapse_rect(&overlay->inspector_panel)
                : debug_title_rect(&overlay->inspector_panel))) {
            overlay->inspector_collapsed = !overlay->inspector_collapsed;
            if (overlay->inspector_collapsed) {
                overlay->inspector_panel.x = (float)viewport_width - debug_inspector_collapsed_width() - 12.0f;
            } else {
                overlay->inspector_panel.x = (float)viewport_width - overlay->inspector_panel.width - 12.0f;
            }
            overlay->inspector_panel.dragging = false;
            handled = true;
        }
        if (!handled && !overlay->inspector_collapsed) {
            (void)debug_handle_panel_drag(&overlay->inspector_panel, input, pressed, released);
        }
    } else if (!has_selection) {
        overlay->inspector_panel.dragging = false;
    }
}

bool debug_overlay_is_pointer_over_ui(
    DebugOverlay* overlay,
    bool has_selection,
    float screen_x,
    float screen_y,
    int viewport_width,
    int viewport_height
) {
    if (overlay == NULL || !overlay->draw_ui) {
        return false;
    }

    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    if (debug_point_in_rect(screen_x, screen_y, debug_panel_rect(&overlay->dashboard_panel))) {
        return true;
    }
    if (has_selection && debug_point_in_rect(
            screen_x,
            screen_y,
            overlay->inspector_collapsed
                ? debug_inspector_collapse_rect(&overlay->inspector_panel)
                : debug_panel_rect(&overlay->inspector_panel))) {
        return true;
    }
    return false;
}
