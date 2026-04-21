#include "Camera.h"

void camera_init(Camera *camera, float x, float y, float view_width, float view_height, float smoothing) {
    if (camera == NULL) {
        return;
    }
    camera->x = x;
    camera->y = y;
    camera->view_width = view_width;
    camera->view_height = view_height;
    camera->viewport_width = view_width;
    camera->viewport_height = view_height;
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

void camera_clamp_to_bounds(Camera* camera, float min_view_width, float min_view_height) {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    float max_camera_x;
    float max_camera_y;

    if (camera == NULL || camera->world_bounds.w <= 0.0f || camera->world_bounds.h <= 0.0f) {
        return;
    }

    min_x = camera->world_bounds.x;
    min_y = camera->world_bounds.y;
    max_x = camera->world_bounds.x + camera->world_bounds.w;
    max_y = camera->world_bounds.y + camera->world_bounds.h;
    camera->view_width = clamp_f(camera->view_width, min_view_width, camera->world_bounds.w);
    camera->view_height = clamp_f(camera->view_height, min_view_height, camera->world_bounds.h);
    max_camera_x = max_x - camera->view_width;
    max_camera_y = max_y - camera->view_height;
    if (max_camera_x < min_x) {
        max_camera_x = min_x;
    }
    if (max_camera_y < min_y) {
        max_camera_y = min_y;
    }
    camera->x = clamp_f(camera->x, min_x, max_camera_x);
    camera->y = clamp_f(camera->y, min_y, max_camera_y);
}

void camera_zoom_at_screen_point(Camera* camera, float zoom_factor, Vec2 anchor_screen, float min_view_width, float min_view_height) {
    float old_view_width;
    float old_view_height;
    float new_view_width;
    float new_view_height;
    float anchor_u;
    float anchor_v;
    Vec2 anchor_world;

    if (camera == NULL || zoom_factor <= 0.0f || camera->viewport_width <= 0.0f || camera->viewport_height <= 0.0f) {
        return;
    }

    old_view_width = camera->view_width;
    old_view_height = camera->view_height;
    if (old_view_width <= 0.0f || old_view_height <= 0.0f) {
        return;
    }

    new_view_width = old_view_width * zoom_factor;
    new_view_height = old_view_height * zoom_factor;
    if (camera->world_bounds.w > 0.0f) {
        new_view_width = clamp_f(new_view_width, min_view_width, camera->world_bounds.w);
    }
    if (camera->world_bounds.h > 0.0f) {
        new_view_height = clamp_f(new_view_height, min_view_height, camera->world_bounds.h);
    }

    anchor_u = clamp_f(anchor_screen.x / camera->viewport_width, 0.0f, 1.0f);
    anchor_v = clamp_f(anchor_screen.y / camera->viewport_height, 0.0f, 1.0f);
    anchor_world.x = camera->x + anchor_u * old_view_width;
    anchor_world.y = camera->y + anchor_v * old_view_height;
    camera->view_width = new_view_width;
    camera->view_height = new_view_height;
    camera->x = anchor_world.x - anchor_u * new_view_width;
    camera->y = anchor_world.y - anchor_v * new_view_height;
    camera_clamp_to_bounds(camera, min_view_width, min_view_height);
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
        float scale_x = camera->view_width > 0.0f ? camera->viewport_width / camera->view_width : 1.0f;
        float scale_y = camera->view_height > 0.0f ? camera->viewport_height / camera->view_height : 1.0f;
        result.x = (result.x - camera->x) * scale_x;
        result.y = (result.y - camera->y) * scale_y;
    }
    return result;
}

Vec2 camera_screen_to_world(const Camera *camera, Vec2 point) {
    Vec2 result = point;
    if (camera != NULL) {
        float scale_x = camera->viewport_width > 0.0f ? camera->view_width / camera->viewport_width : 1.0f;
        float scale_y = camera->viewport_height > 0.0f ? camera->view_height / camera->viewport_height : 1.0f;
        result.x = result.x * scale_x + camera->x;
        result.y = result.y * scale_y + camera->y;
    }
    return result;
}

Vec2 camera_world_size_to_screen(const Camera *camera, Vec2 size) {
    Vec2 result = size;
    if (camera != NULL) {
        float scale_x = camera->view_width > 0.0f ? camera->viewport_width / camera->view_width : 1.0f;
        float scale_y = camera->view_height > 0.0f ? camera->viewport_height / camera->view_height : 1.0f;
        result.x *= scale_x;
        result.y *= scale_y;
    }
    return result;
}
