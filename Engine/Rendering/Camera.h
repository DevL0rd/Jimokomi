#ifndef JIMOKOMI_ENGINE_RENDERING_CAMERA_H
#define JIMOKOMI_ENGINE_RENDERING_CAMERA_H

#include "RenderCommon.h"

typedef struct Camera {
    float x;
    float y;
    float view_width;
    float view_height;
    float smoothing;
    Rect world_bounds;
} Camera;

void camera_init(Camera *camera, float x, float y, float view_width, float view_height, float smoothing);
void camera_set_world_bounds(Camera *camera, Rect bounds);
void camera_follow_position(Camera *camera, float target_x, float target_y, float dt_seconds, Vec2 lead, float smoothing_override);
Vec2 camera_world_to_screen(const Camera *camera, Vec2 point);
Vec2 camera_screen_to_world(const Camera *camera, Vec2 point);

#endif
