#include "DebugOverlay.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

enum {
    DEBUG_TITLE_BAR_HEIGHT = 20
};

static bool debug_point_in_rect(float x, float y, Rect rect) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

static Rect debug_panel_rect(const DebugPanelState *panel) {
    Rect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (panel != NULL) {
        rect.x = panel->x;
        rect.y = panel->y;
        rect.w = panel->width;
        rect.h = panel->height;
    }
    return rect;
}

static Rect debug_title_rect(const DebugPanelState *panel) {
    Rect rect = debug_panel_rect(panel);
    rect.h = (float)DEBUG_TITLE_BAR_HEIGHT;
    return rect;
}

static void debug_draw_bar(
    Target *target,
    float x,
    float y,
    float width,
    float height,
    float value,
    float max_value,
    Color32 fill_color
) {
    float ratio = max_value > 0.0f ? clamp_f(value / max_value, 0.0f, 1.0f) : 0.0f;
    target_rect_filled(target, (Rect){ x, y, width, height }, (Color32){ 0x182230U });
    target_rect_filled(target, (Rect){ x + 1.0f, y + 1.0f, (width - 2.0f) * ratio, height - 2.0f }, fill_color);
    target_rect(target, (Rect){ x, y, width, height }, (Color32){ 0x5fd0ffU });
}

static void debug_draw_history(
    Target *target,
    const double *values,
    size_t count,
    float max_value,
    float x,
    float y,
    float width,
    float height,
    Color32 line_color
) {
    size_t index;
    if (values == NULL || count < 2U || max_value <= 0.0f) {
        return;
    }

    target_rect_filled(target, (Rect){ x, y, width, height }, (Color32){ 0x101722U });
    target_rect(target, (Rect){ x, y, width, height }, (Color32){ 0x31445dU });

    for (index = 1U; index < count; ++index) {
        float x0 = x + ((float)(index - 1U) / (float)(count - 1U)) * (width - 2.0f) + 1.0f;
        float x1 = x + ((float)index / (float)(count - 1U)) * (width - 2.0f) + 1.0f;
        float y0 = y + height - 2.0f - ((float)clamp_f((float)values[index - 1U], 0.0f, max_value) / max_value) * (height - 3.0f);
        float y1 = y + height - 2.0f - ((float)clamp_f((float)values[index], 0.0f, max_value) / max_value) * (height - 3.0f);
        target_line(target, x0, y0, x1, y1, line_color);
    }
}

static void debug_draw_panel_frame(
    Target *target,
    const DebugPanelState *panel,
    const char *title
) {
    Rect rect = debug_panel_rect(panel);
    Rect title_rect = debug_title_rect(panel);

    target_rect_filled(target, rect, (Color32){ 0x0d1520U });
    target_rect(target, rect, (Color32){ 0x7ee0ffU });
    target_rect_filled(target, title_rect, (Color32){ 0x132338U });
    target_line(target, rect.x, rect.y + title_rect.h, rect.x + rect.w, rect.y + title_rect.h, (Color32){ 0x7ee0ffU });
    target_text(target, rect.x + 8.0f, rect.y + 4.0f, title != NULL ? title : "Panel", (Color32){ 0xeaf7ffU });
}

static void debug_overlay_ensure_layout(
    DebugOverlay *overlay,
    bool has_selection,
    int viewport_width,
    int viewport_height
) {
    if (overlay == NULL) {
        return;
    }

    if (!overlay->layout_initialized) {
        overlay->dashboard_panel.width = 320.0f;
        overlay->dashboard_panel.height = 198.0f;
        overlay->dashboard_panel.x = (float)viewport_width - overlay->dashboard_panel.width - 12.0f;
        overlay->dashboard_panel.y = 12.0f;

        overlay->inspector_panel.width = 320.0f;
        overlay->inspector_panel.height = 152.0f;
        overlay->inspector_panel.x = overlay->dashboard_panel.x;
        overlay->inspector_panel.y = overlay->dashboard_panel.y + overlay->dashboard_panel.height + 12.0f;
        overlay->layout_initialized = true;
    }

    overlay->dashboard_panel.x = clamp_f(overlay->dashboard_panel.x, 0.0f, (float)viewport_width - overlay->dashboard_panel.width);
    overlay->dashboard_panel.y = clamp_f(overlay->dashboard_panel.y, 0.0f, (float)viewport_height - overlay->dashboard_panel.height);
    overlay->inspector_panel.x = clamp_f(overlay->inspector_panel.x, 0.0f, (float)viewport_width - overlay->inspector_panel.width);
    overlay->inspector_panel.y = clamp_f(overlay->inspector_panel.y, 0.0f, (float)viewport_height - overlay->inspector_panel.height);

    if (!has_selection) {
        overlay->inspector_panel.dragging = false;
    }
}

static bool debug_handle_panel_drag(
    DebugPanelState *panel,
    const EngineInput *input,
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

static void debug_draw_entity(Target *target, const Camera *camera, const DebugEntityView *entity) {
    Vec2 screen_position;
    Vec2 heading;
    Vec2 right;
    Vec2 velocity;
    float speed;
    float velocity_scale;
    Color32 outline = { 0x49ffaeU };
    Color32 heading_color = { 0x7ee0ffU };
    Color32 velocity_color = { 0xffd166U };

    if (target == NULL || camera == NULL || entity == NULL) {
        return;
    }

    if (entity->is_selected) {
        outline.value = 0x00ffffU;
    } else if (entity->is_sleeping) {
        outline.value = 0x6b7890U;
    }

    screen_position = camera_world_to_screen(camera, entity->position);
    if (entity->is_circle) {
        target_circle(target, screen_position, entity->radius, outline);
    } else {
        target_rect(target, (Rect){
            screen_position.x - entity->extent_x,
            screen_position.y - entity->extent_y,
            entity->extent_x * 2.0f,
            entity->extent_y * 2.0f
        }, outline);
    }

    heading.x = cosf(entity->angle_radians);
    heading.y = sinf(entity->angle_radians);
    right.x = -heading.y;
    right.y = heading.x;
    target_line(
        target,
        screen_position.x,
        screen_position.y,
        screen_position.x + heading.x * 16.0f,
        screen_position.y + heading.y * 16.0f,
        heading_color
    );
    target_line(
        target,
        screen_position.x - right.x * 6.0f,
        screen_position.y - right.y * 6.0f,
        screen_position.x + right.x * 6.0f,
        screen_position.y + right.y * 6.0f,
        heading_color
    );

    speed = sqrtf(entity->velocity.x * entity->velocity.x + entity->velocity.y * entity->velocity.y);
    velocity_scale = clamp_f(speed * 0.55f, 0.0f, 20.0f);
    velocity.x = speed > 0.001f ? (entity->velocity.x / speed) * velocity_scale : 0.0f;
    velocity.y = speed > 0.001f ? (entity->velocity.y / speed) * velocity_scale : 0.0f;
    target_line(
        target,
        screen_position.x,
        screen_position.y,
        screen_position.x + velocity.x,
        screen_position.y + velocity.y,
        velocity_color
    );

    if (entity->is_selected) {
        target_text(target, screen_position.x + 10.0f, screen_position.y - 18.0f, "T", (Color32){ 0x00ffffU });
    } else if (entity->is_sleeping) {
        target_text(target, screen_position.x + 10.0f, screen_position.y - 18.0f, "S", (Color32){ 0xa8b4c8U });
    } else if (entity->is_moving) {
        target_text(target, screen_position.x + 10.0f, screen_position.y - 18.0f, "M", (Color32){ 0xffd166U });
    }
}

static bool debug_entity_visible(const Camera* camera, const DebugEntityView* entity) {
    float extent_x;
    float extent_y;

    if (camera == NULL || entity == NULL) {
        return false;
    }

    extent_x = entity->is_circle ? entity->radius : entity->extent_x;
    extent_y = entity->is_circle ? entity->radius : entity->extent_y;

    return entity->position.x + extent_x >= camera->x &&
           entity->position.y + extent_y >= camera->y &&
           entity->position.x - extent_x <= camera->x + camera->view_width &&
           entity->position.y - extent_y <= camera->y + camera->view_height;
}

void debug_overlay_init(DebugOverlay *overlay) {
    if (overlay == NULL) {
        return;
    }
    memset(overlay, 0, sizeof(*overlay));
    overlay->enabled = true;
    overlay->draw_world_gizmos = false;
    overlay->draw_ui_bounds = true;
    overlay->world_refresh_interval_ms = 250U;
    overlay->last_visible_entity_count = 0U;
    overlay->last_active_collision_count = 0U;
}

void debug_overlay_dispose(DebugOverlay *overlay) {
    if (overlay == NULL) {
        return;
    }
    if (overlay->world_surface != NULL &&
        overlay->world_surface_backend != NULL &&
        overlay->world_surface_backend->destroy_surface != NULL) {
        overlay->world_surface_backend->destroy_surface(
            overlay->world_surface_backend->userdata,
            overlay->world_surface
        );
    }
    memset(overlay, 0, sizeof(*overlay));
}

void debug_overlay_toggle(DebugOverlay *overlay) {
    if (overlay != NULL) {
        overlay->enabled = !overlay->enabled;
        overlay->dashboard_panel.dragging = false;
        overlay->inspector_panel.dragging = false;
    }
}

void debug_overlay_handle_input(
    DebugOverlay *overlay,
    const EngineInput *input,
    bool has_selection,
    int viewport_width,
    int viewport_height
) {
    bool pressed;
    bool released;
    bool handled;

    if (overlay == NULL || input == NULL || !overlay->enabled) {
        return;
    }

    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    pressed = EngineInput_was_mouse_pressed(input, 1U);
    released = EngineInput_was_mouse_released(input, 1U);

    handled = debug_handle_panel_drag(&overlay->dashboard_panel, input, pressed, released);
    if (!handled && has_selection) {
        (void)debug_handle_panel_drag(&overlay->inspector_panel, input, pressed, released);
    } else if (!has_selection) {
        overlay->inspector_panel.dragging = false;
    }
}

bool debug_overlay_is_pointer_over_ui(
    DebugOverlay *overlay,
    bool has_selection,
    float screen_x,
    float screen_y,
    int viewport_width,
    int viewport_height
) {
    if (overlay == NULL || !overlay->enabled) {
        return false;
    }

    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    if (debug_point_in_rect(screen_x, screen_y, debug_panel_rect(&overlay->dashboard_panel))) {
        return true;
    }
    if (has_selection && debug_point_in_rect(screen_x, screen_y, debug_panel_rect(&overlay->inspector_panel))) {
        return true;
    }
    return false;
}

void debug_overlay_draw_world(
    DebugOverlay *overlay,
    RenderBackend *backend,
    uint64_t now_ms,
    const Camera *camera,
    const DebugEntityView *entities,
    size_t entity_count,
    const DebugCollisionView *collisions,
    size_t collision_count
) {
    size_t index;
    Target target;
    bool should_redraw;

    if (overlay == NULL || !overlay->enabled || !overlay->draw_world_gizmos || backend == NULL || camera == NULL) {
        return;
    }

    if (backend->create_surface == NULL ||
        backend->destroy_surface == NULL ||
        backend->set_target == NULL ||
        backend->reset_target == NULL ||
        backend->clear == NULL) {
        return;
    }

    if (overlay->world_surface == NULL ||
        overlay->world_surface_width != (int)camera->view_width ||
        overlay->world_surface_height != (int)camera->view_height) {
        if (overlay->world_surface != NULL) {
            backend->destroy_surface(backend->userdata, overlay->world_surface);
        }
        overlay->world_surface = backend->create_surface(
            backend->userdata,
            (int)camera->view_width,
            (int)camera->view_height
        );
        overlay->world_surface_backend = backend;
        overlay->world_surface_width = (int)camera->view_width;
        overlay->world_surface_height = (int)camera->view_height;
        overlay->last_world_redraw_ms = 0U;
    }

    if (overlay->world_surface == NULL) {
        return;
    }

    should_redraw = overlay->last_world_redraw_ms == 0U ||
        now_ms >= overlay->last_world_redraw_ms + overlay->world_refresh_interval_ms;

    if (should_redraw) {
        overlay->last_visible_entity_count = 0U;
        overlay->last_active_collision_count = 0U;

        backend->set_target(backend->userdata, overlay->world_surface);
        backend->clear(backend->userdata, (Color32){ 0x00000000U });
        target_init(&target, backend, 0.0f, 0.0f);

        for (index = 0U; index < entity_count; ++index) {
            if (!debug_entity_visible(camera, &entities[index])) {
                continue;
            }
            overlay->last_visible_entity_count += 1U;
            debug_draw_entity(&target, camera, &entities[index]);
        }
        for (index = 0U; index < collision_count; ++index) {
            Vec2 a;
            Vec2 b;
            Vec2 c;
            Vec2 n;
            Color32 color;
            if (!collisions[index].active) {
                continue;
            }
            overlay->last_active_collision_count += 1U;
            a = camera_world_to_screen(camera, collisions[index].point_a);
            b = camera_world_to_screen(camera, collisions[index].point_b);
            c = camera_world_to_screen(camera, collisions[index].contact_point);
            n = collisions[index].contact_normal;
            color.value = collisions[index].sleeping ? 0x666666U : (collisions[index].sensor ? 0xff00ffU : 0xffff00U);
            target_line(&target, a.x, a.y, b.x, b.y, color);
            target_circle_filled(&target, c, 1.5f, color);
            target_line(&target, c.x, c.y, c.x + n.x * 8.0f, c.y + n.y * 8.0f, color);
        }

        backend->reset_target(backend->userdata);
        overlay->last_world_redraw_ms = now_ms;
    }

    target_init(&target, backend, 0.0f, 0.0f);
    target_surface(&target, overlay->world_surface, 0.0f, 0.0f);
}

void debug_overlay_draw_ui(
    DebugOverlay *overlay,
    RenderBackend *backend,
    const DebugOverlaySnapshot *snapshot,
    const EngineStats *stats,
    const DebugEntityView *selected_entity,
    int viewport_width,
    int viewport_height
) {
    Target target;
    char text[128];
    float left;
    float top;
    bool has_selection;

    if (overlay == NULL || !overlay->enabled || backend == NULL || snapshot == NULL || stats == NULL) {
        return;
    }

    has_selection = selected_entity != NULL;
    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    target_init(&target, backend, 0.0f, 0.0f);

    debug_draw_panel_frame(&target, &overlay->dashboard_panel, "Debug");
    left = overlay->dashboard_panel.x + 10.0f;
    top = overlay->dashboard_panel.y + 30.0f;

    snprintf(text, sizeof(text), "FPS %.1f", snapshot->fps);
    target_text(&target, left, top, text, (Color32){ 0xeaf7ffU });
    debug_draw_bar(&target, left + 92.0f, top - 1.0f, 208.0f, 12.0f, snapshot->fps, 60.0f, (Color32){ 0x49ffaeU });

    top += 22.0f;
    snprintf(text, sizeof(text), "Upd %.2fms", snapshot->update_ms);
    target_text(&target, left, top, text, (Color32){ 0xeaf7ffU });
    debug_draw_bar(&target, left + 92.0f, top - 1.0f, 208.0f, 12.0f, snapshot->update_ms, 16.67f, (Color32){ 0x7ee0ffU });

    top += 22.0f;
    snprintf(text, sizeof(text), "Drw %.2fms", snapshot->draw_ms);
    target_text(&target, left, top, text, (Color32){ 0xeaf7ffU });
    debug_draw_bar(&target, left + 92.0f, top - 1.0f, 208.0f, 12.0f, snapshot->draw_ms, 16.67f, (Color32){ 0xf28482U });

    top += 22.0f;
    snprintf(text, sizeof(text), "Frm %.2fms", (float)stats->last_frame_ms);
    target_text(&target, left, top, text, (Color32){ 0xeaf7ffU });
    debug_draw_bar(&target, left + 92.0f, top - 1.0f, 208.0f, 12.0f, (float)stats->last_frame_ms, 16.67f, (Color32){ 0xffd166U });

    top += 24.0f;
    target_text(&target, left, top, "History", (Color32){ 0xeaf7ffU });
    top += 14.0f;
    debug_draw_history(&target, stats->frame_history_ms, stats->history_count, 50.0f, left, top, 290.0f, 40.0f, (Color32){ 0xffd166U });

    top += 50.0f;
    snprintf(text, sizeof(text), "Phy %.2fms", snapshot->physics_ms);
    target_text(&target, left, top, text, (Color32){ 0xb6c6d9U });
    target_text(&target, left, top + 16.0f, "F1 toggle  Tab cycle  Click inspect", (Color32){ 0x7fa2bfU });

    if (has_selection) {
        debug_draw_panel_frame(&target, &overlay->inspector_panel, "Inspector");
        left = overlay->inspector_panel.x + 10.0f;
        top = overlay->inspector_panel.y + 30.0f;

        snprintf(text, sizeof(text), "Entity %llu", (unsigned long long)selected_entity->id);
        target_text(&target, left, top, text, (Color32){ 0xeaf7ffU });
        top += 18.0f;
        snprintf(text, sizeof(text), "Pos %.2f  %.2f", selected_entity->position.x, selected_entity->position.y);
        target_text(&target, left, top, text, (Color32){ 0xb6c6d9U });
        top += 16.0f;
        snprintf(text, sizeof(text), "Vel %.2f  %.2f", selected_entity->velocity.x, selected_entity->velocity.y);
        target_text(&target, left, top, text, (Color32){ 0xb6c6d9U });
        top += 16.0f;
        snprintf(text, sizeof(text), "Ang %.2f", selected_entity->angle_radians);
        target_text(&target, left, top, text, (Color32){ 0xb6c6d9U });
        top += 16.0f;
        if (selected_entity->is_circle) {
            snprintf(text, sizeof(text), "Circle r %.2f", selected_entity->radius);
        } else {
            snprintf(text, sizeof(text), "Box %.2f x %.2f", selected_entity->extent_x, selected_entity->extent_y);
        }
        target_text(&target, left, top, text, (Color32){ 0xb6c6d9U });
        top += 16.0f;
        snprintf(text, sizeof(text), "Sleep %s  Move %s", selected_entity->is_sleeping ? "Y" : "N", selected_entity->is_moving ? "Y" : "N");
        target_text(&target, left, top, text, (Color32){ 0xb6c6d9U });
    }
}
