#include "Target.h"

#include <math.h>

static Rect target_offset_rect(Target *target, Rect rect) {
    return (Rect){
        target->origin.x + rect.x * target->scale.x,
        target->origin.y + rect.y * target->scale.y,
        rect.w * target->scale.x,
        rect.h * target->scale.y
    };
}

static Vec2 target_offset_vec2(Target *target, Vec2 point) {
    point.x = target->origin.x + point.x * target->scale.x;
    point.y = target->origin.y + point.y * target->scale.y;
    return point;
}

static float target_scale_radius(const Target* target, float radius) {
    if (target == NULL) {
        return radius;
    }
    return radius * fminf(fabsf(target->scale.x), fabsf(target->scale.y));
}

void target_init(Target *target, RenderBackend *backend, float origin_x, float origin_y) {
    target_init_scaled(target, backend, origin_x, origin_y, 1.0f, 1.0f);
}

void target_init_scaled(Target *target, RenderBackend *backend, float origin_x, float origin_y, float scale_x, float scale_y) {
    if (target == NULL) {
        return;
    }
    target->backend = backend;
    target->origin.x = origin_x;
    target->origin.y = origin_y;
    target->scale.x = scale_x;
    target->scale.y = scale_y;
}

void target_line(Target *target, float x0, float y0, float x1, float y1, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_line == NULL) {
        return;
    }
    target->backend->draw_line(
        target->backend->userdata,
        target->origin.x + x0 * target->scale.x,
        target->origin.y + y0 * target->scale.y,
        target->origin.x + x1 * target->scale.x,
        target->origin.y + y1 * target->scale.y,
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

void target_triangle_filled(Target *target, Vec2 a, Vec2 b, Vec2 c, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_triangle_filled == NULL) {
        return;
    }
    target->backend->draw_triangle_filled(
        target->backend->userdata,
        target_offset_vec2(target, a),
        target_offset_vec2(target, b),
        target_offset_vec2(target, c),
        color
    );
}

void target_circle(Target *target, Vec2 center, float radius, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_circle == NULL) {
        return;
    }
    target->backend->draw_circle(target->backend->userdata, target_offset_vec2(target, center), target_scale_radius(target, radius), color, false);
}

void target_circle_filled(Target *target, Vec2 center, float radius, Color32 color) {
    if (target == NULL || target->backend == NULL || target->backend->draw_circle == NULL) {
        return;
    }
    target->backend->draw_circle(target->backend->userdata, target_offset_vec2(target, center), target_scale_radius(target, radius), color, true);
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
    target->backend->draw_text(target->backend->userdata, target->origin.x + x * target->scale.x, target->origin.y + y * target->scale.y, text, color);
}

void target_texture(Target *target, const Texture *texture, float x, float y) {
    if (target == NULL || target->backend == NULL || target->backend->draw_texture == NULL) {
        return;
    }
    target->backend->draw_texture(target->backend->userdata, texture, target->origin.x + x * target->scale.x, target->origin.y + y * target->scale.y);
}

void target_texture_tinted(Target *target, const Texture *texture, float x, float y, Color32 tint) {
    if (target == NULL || target->backend == NULL || target->backend->draw_texture_tinted == NULL) {
        return;
    }
    target->backend->draw_texture_tinted(
        target->backend->userdata,
        texture,
        target->origin.x + x * target->scale.x,
        target->origin.y + y * target->scale.y,
        tint
    );
}

void target_texture_ex(Target *target, const Texture *texture, Rect dest, Vec2 origin, float rotation_degrees) {
    if (target == NULL || target->backend == NULL || target->backend->draw_texture_ex == NULL) {
        return;
    }
    dest = target_offset_rect(target, dest);
    origin.x *= target->scale.x;
    origin.y *= target->scale.y;
    target->backend->draw_texture_ex(target->backend->userdata, texture, dest, origin, rotation_degrees);
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
        target->origin.x + x * target->scale.x,
        target->origin.y + y * target->scale.y,
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
