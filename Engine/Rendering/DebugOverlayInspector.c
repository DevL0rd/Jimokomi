#include "DebugOverlayInternal.h"

#include "DebugOverlayUiInternal.h"
#include <stdio.h>

void debug_overlay_draw_inspector_contents(
    DebugOverlay* overlay,
    Target* target,
    const DebugEntityView* selected_entity
) {
    char text[128];
    float left;
    float top;
    float right;

    if (overlay == NULL || target == NULL || selected_entity == NULL) {
        return;
    }

    if (overlay->ui->inspector_collapsed) {
        debug_draw_inspector_rail(
            target,
            &overlay->ui->inspector_panel,
            overlay->ui->hovered_ui_region == DEBUG_UI_HOVER_INSPECTOR
        );
        return;
    }

    debug_draw_panel_frame(
        target,
        &overlay->ui->inspector_panel,
        "Inspector",
        overlay->ui->hovered_ui_region == DEBUG_UI_HOVER_INSPECTOR
    );
    left = overlay->ui->inspector_panel.x + 12.0f;
    top = overlay->ui->inspector_panel.y + 30.0f;
    right = overlay->ui->inspector_panel.x + overlay->ui->inspector_panel.width - 12.0f;

    snprintf(text, sizeof(text), "Entity %llu", (unsigned long long)selected_entity->id);
    target_text(target, left, top, text, (Color32){ 0xf2f8fcU });
    snprintf(text, sizeof(text), "%s", selected_entity->is_sleeping ? "Sleeping" : (selected_entity->is_moving ? "Moving" : "Idle"));
    target_text(target, right - 54.0f, top, text, selected_entity->is_sleeping ? (Color32){ 0x7f99adU } : (Color32){ 0x8fe8c4U });
    top += 18.0f;

    target_text(target, left, top, "Position", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%.1f  %.1f", selected_entity->position.x, selected_entity->position.y);
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Velocity", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%.1f  %.1f", selected_entity->velocity.x, selected_entity->velocity.y);
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Angle", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%.2f rad", selected_entity->angle_radians);
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Shape", (Color32){ 0x7f99adU });
    if (selected_entity->is_circle) {
        snprintf(text, sizeof(text), "Circle %.1f", selected_entity->radius);
    } else {
        snprintf(text, sizeof(text), "Box %.1f x %.1f", selected_entity->extent_x, selected_entity->extent_y);
    }
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
    top += 16.0f;

    target_text(target, left, top, "Flags", (Color32){ 0x7f99adU });
    snprintf(text, sizeof(text), "%s  %s",
        selected_entity->is_selected ? "Selected" : "Unselected",
        selected_entity->is_moving ? "Moving" : "Still");
    target_text(target, left + 82.0f, top, text, (Color32){ 0xdce7efU });
}
