#include "DebugOverlayInternal.h"

void debug_draw_panel_frame(
    Target* target,
    const DebugPanelState* panel,
    const char* title,
    bool hovered
) {
    Rect rect = debug_panel_rect(panel);
    Rect title_rect = debug_title_rect(panel);
    Color32 title_fill = hovered ? (Color32){ 0x132435U } : (Color32){ 0x0c1823U };

    target_rect_filled(target, rect, (Color32){ 0x08111aU });
    target_rect(target, rect, (Color32){ 0x22364aU });
    target_rect_filled(target, title_rect, title_fill);
    target_line(target, rect.x, rect.y + title_rect.h, rect.x + rect.w, rect.y + title_rect.h, (Color32){ 0x1a2b3aU });
    target_text(target, rect.x + 10.0f, rect.y + 4.0f, title != NULL ? title : "Panel", (Color32){ 0xf1f7fbU });
}

void debug_draw_inspector_rail(Target* target, const DebugPanelState* panel, bool hovered) {
    Rect rail_rect = debug_inspector_collapse_rect(panel);
    Color32 title_fill = hovered ? (Color32){ 0x132435U } : (Color32){ 0x0c1823U };

    if (target == NULL || panel == NULL) {
        return;
    }

    target_rect_filled(target, rail_rect, (Color32){ 0x08111aU });
    target_rect(target, rail_rect, (Color32){ 0x22364aU });
    target_rect_filled(target, (Rect){ rail_rect.x, rail_rect.y, rail_rect.w, debug_title_bar_height() }, title_fill);
    target_line(target, rail_rect.x, rail_rect.y + debug_title_bar_height(), rail_rect.x + rail_rect.w, rail_rect.y + debug_title_bar_height(), (Color32){ 0x1a2b3aU });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 32.0f, "I", (Color32){ 0xf1f7fbU });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 52.0f, "N", (Color32){ 0x9ab0c1U });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 72.0f, "F", (Color32){ 0x9ab0c1U });
    target_text(target, rail_rect.x + 7.0f, rail_rect.y + 92.0f, "O", (Color32){ 0x9ab0c1U });
}
