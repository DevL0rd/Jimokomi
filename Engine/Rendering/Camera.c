#include "Camera.h"

void camera_init(Camera *camera, float x, float y, float view_width, float view_height, float smoothing) {
    if (camera == NULL) {
        return;
    }
    camera->x = x;
    camera->y = y;
    camera->view_width = view_width;
    camera->view_height = view_height;
    camera->smoothing = smoothing;
    camera->world_bounds.x = 0.0f;
    camera->world_bounds.y = 0.0f;
    camera->world_bounds.w = 0.0f;
    camera->world_bounds.h = 0.0f;
}

void camera_set_world_bounds(Camera *camera, Rect bounds) {
    if (camera == NULL) {
        return;
    }
    camera->world_bounds = bounds;
    camera->x = clamp_f(camera->x, bounds.x, bounds.x + bounds.w);
    camera->y = clamp_f(camera->y, bounds.y, bounds.y + bounds.h);
}

void camera_follow_position(Camera *camera, float target_x, float target_y, float dt_seconds, Vec2 lead, float smoothing_override) {
    Vec2 desired;
    Vec2 current;
    Vec2 next;
    float smoothing;

    if (camera == NULL) {
        return;
    }

    desired.x = target_x - camera->view_width * 0.5f + lead.x;
    desired.y = target_y - camera->view_height * 0.5f + lead.y;
    smoothing = smoothing_override > 0.0f ? smoothing_override : camera->smoothing;
    current.x = camera->x;
    current.y = camera->y;

    if (smoothing <= 0.0f) {
        next = desired;
    } else {
        next = vec2_lerp(current, desired, exp_smoothing_alpha(dt_seconds, smoothing));
    }

    camera->x = next.x;
    camera->y = next.y;

    camera->x = clamp_f(camera->x, camera->world_bounds.x, camera->world_bounds.x + camera->world_bounds.w);
    camera->y = clamp_f(camera->y, camera->world_bounds.y, camera->world_bounds.y + camera->world_bounds.h);
}

Vec2 camera_world_to_screen(const Camera *camera, Vec2 point) {
    Vec2 result = point;
    if (camera != NULL) {
        result.x -= camera->x;
        result.y -= camera->y;
    }
    return result;
}

Vec2 camera_screen_to_world(const Camera *camera, Vec2 point) {
    Vec2 result = point;
    if (camera != NULL) {
        result.x += camera->x;
        result.y += camera->y;
    }
    return result;
}
