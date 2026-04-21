#include "CameraFollowSystem.h"

#include "../SceneCameraFollowInternal.h"
#include "../SceneInternal.h"
#include "../SceneLifecycleInternal.h"
#include "../SceneSpatialInternal.h"
#include "../Entity.h"
#include "../Components/CameraTargetComponent.h"
#include "../Components/TransformComponent.h"

static float CameraFollowSystem_Lerp(float current, float target, float alpha)
{
    return current + ((target - current) * alpha);
}

void CameraFollowSystem_Update(struct Scene* scene, float dt_seconds)
{
    Entity* entity = NULL;
    CameraTargetComponent* camera_target = NULL;
    TransformComponent* transform = NULL;
    float smoothing = 0.0f;
    float target_x = 0.0f;
    float target_y = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;

    if (scene == NULL || scene->lifecycle->is_overlay)
    {
        return;
    }

    entity = scene->camera_follow->camera_target_entity;
    if (!Entity_IsActive(entity))
    {
        return;
    }

    camera_target = (CameraTargetComponent*)Entity_GetComponent(entity, COMPONENT_CAMERA_TARGET);
    transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
    if (camera_target == NULL || transform == NULL || !camera_target->follow)
    {
        scene->camera_follow->camera_target_entity = NULL;
        scene->camera_follow->last_camera_follow_entity = NULL;
        return;
    }

    smoothing = camera_target->smoothing > 0.0f ? camera_target->smoothing : scene->spatial->view.smoothing;
    target_x = transform->x + camera_target->lead_x - (scene->spatial->view.view_width * 0.5f);
    target_y = transform->y + camera_target->lead_y - (scene->spatial->view.view_height * 0.5f);
    if (scene->camera_follow->last_camera_follow_entity == entity &&
        scene->camera_follow->last_camera_follow_target_x == target_x &&
        scene->camera_follow->last_camera_follow_target_y == target_y &&
        scene->camera_follow->last_camera_follow_view_width == scene->spatial->view.view_width &&
        scene->camera_follow->last_camera_follow_view_height == scene->spatial->view.view_height)
    {
        scene->spatial->view.previous_x = scene->spatial->view.x;
        scene->spatial->view.previous_y = scene->spatial->view.y;
        return;
    }
    scene->spatial->view.previous_x = scene->spatial->view.x;
    scene->spatial->view.previous_y = scene->spatial->view.y;

    if (smoothing > 0.0f)
    {
        float alpha = dt_seconds / (dt_seconds + smoothing);
        scene->spatial->view.x = CameraFollowSystem_Lerp(scene->spatial->view.x, target_x, alpha);
        scene->spatial->view.y = CameraFollowSystem_Lerp(scene->spatial->view.y, target_y, alpha);
    }
    else
    {
        scene->spatial->view.x = target_x;
        scene->spatial->view.y = target_y;
    }

    max_x = scene->spatial->view.world_max_x - scene->spatial->view.view_width;
    max_y = scene->spatial->view.world_max_y - scene->spatial->view.view_height;

    if (scene->spatial->view.has_world_bounds)
    {
        if (scene->spatial->view.x < scene->spatial->view.world_min_x)
        {
            scene->spatial->view.x = scene->spatial->view.world_min_x;
        }
        if (scene->spatial->view.y < scene->spatial->view.world_min_y)
        {
            scene->spatial->view.y = scene->spatial->view.world_min_y;
        }
        if (scene->spatial->view.x > max_x)
        {
            scene->spatial->view.x = max_x;
        }
        if (scene->spatial->view.y > max_y)
        {
            scene->spatial->view.y = max_y;
        }
    }

    scene->camera_follow->last_camera_follow_entity = entity;
    scene->camera_follow->last_camera_follow_target_x = target_x;
    scene->camera_follow->last_camera_follow_target_y = target_y;
    scene->camera_follow->last_camera_follow_view_width = scene->spatial->view.view_width;
    scene->camera_follow->last_camera_follow_view_height = scene->spatial->view.view_height;
}
