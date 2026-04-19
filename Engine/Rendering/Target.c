#include "Target.h"

static Rect target_offset_rect(Target *target, Rect rect) {
    rect.x += target->origin.x;
    rect.y += target->origin.y;
    return rect;
}

static Vec2 target_offset_vec2(Target *target, Vec2 point) {
    point.x += target->origin.x;
    point.y += target->origin.y;
    return point;
}

void target_init(Target *target, RenderBackend *backend, float origin_x, float origin_y) {
    if (target == NULL) {
        return;
    }
    target->backend = backend;
    target->origin.x = origin_x;
    target->origin.y = origin_y;
}

void target_line(Target *target, float x0, float y0, float x1, float y1, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_line == NULL) {
        return;
    }
    target->backend->draw_line(
        target->backend->userdata,
        x0 + target->origin.x,
        y0 + target->origin.y,
        x1 + target->origin.x,
        y1 + target->origin.y,
        color
    );
}

void target_rect(Target *target, Rect rect, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_rect == NULL) {
        return;
    }
    target->backend->draw_rect(target->backend->userdata, target_offset_rect(target, rect), color);
}

void target_rect_filled(Target *target, Rect rect, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_rect_filled == NULL) {
        return;
    }
    target->backend->draw_rect_filled(target->backend->userdata, target_offset_rect(target, rect), color);
}

void target_circle(Target *target, Vec2 center, float radius, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_circle == NULL) {
        return;
    }
    target->backend->draw_circle(target->backend->userdata, target_offset_vec2(target, center), radius, color, false);
}

void target_circle_filled(Target *target, Vec2 center, float radius, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_circle == NULL) {
        return;
    }
    target->backend->draw_circle(target->backend->userdata, target_offset_vec2(target, center), radius, color, true);
}

void target_oval(Target *target, Rect rect, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_oval == NULL) {
        return;
    }
    target->backend->draw_oval(target->backend->userdata, target_offset_rect(target, rect), color, false);
}

void target_oval_filled(Target *target, Rect rect, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_oval == NULL) {
        return;
    }
    target->backend->draw_oval(target->backend->userdata, target_offset_rect(target, rect), color, true);
}

void target_text(Target *target, float x, float y, const char *text, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_text == NULL) {
        return;
    }
    target->backend->draw_text(target->backend->userdata, x + target->origin.x, y + target->origin.y, text, color);
}

void target_surface(Target *target, const Surface *surface, float x, float y) {
    if (target == NULL || target->backend == NULL || target->backend->draw_surface == NULL) {
        return;
    }
    target->backend->draw_surface(target->backend->userdata, surface, x + target->origin.x, y + target->origin.y);
}

void target_surface_ex(Target *target, const Surface *surface, Rect dest, Vec2 origin, float rotation_degrees) {
    if (target == NULL || target->backend == NULL || target->backend->draw_surface_ex == NULL) {
        return;
    }
    dest.x += target->origin.x;
    dest.y += target->origin.y;
    target->backend->draw_surface_ex(target->backend->userdata, surface, dest, origin, rotation_degrees);
}

void target_tilemap(Target *target, const void *source, int tile_x, int tile_y, float x, float y, int width_tiles, int height_tiles) {
    if (target == NULL || target->backend == NULL || target->backend->draw_tilemap == NULL) {
        return;
    }
    target->backend->draw_tilemap(
        target->backend->userdata,
        source,
        tile_x,
        tile_y,
        x + target->origin.x,
        y + target->origin.y,
        width_tiles,
        height_tiles
    );
}

void target_push_clip(Target *target, Rect rect) {
    if (target == NULL || target->backend == NULL || target->backend->push_clip == NULL) {
        return;
    }
    target->backend->push_clip(target->backend->userdata, target_offset_rect(target, rect));
}

void target_pop_clip(Target *target) {
    if (target == NULL || target->backend == NULL || target->backend->pop_clip == NULL) {
        return;
    }
    target->backend->pop_clip(target->backend->userdata);
}
