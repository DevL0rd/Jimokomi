#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_CAMERA_FOLLOW_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_CAMERA_FOLLOW_INTERNAL_H

#include "SceneInternal.h"

struct SceneCameraFollowState {
    struct Entity* camera_target_entity;
    struct Entity* last_camera_follow_entity;
    float last_camera_follow_target_x;
    float last_camera_follow_target_y;
    float last_camera_follow_view_width;
    float last_camera_follow_view_height;
};

#endif
