#include "CameraFollowSystem.h"

#include "../Scene.h"
#include "../Entity.h"
#include "../Components/CameraTargetComponent.h"
#include "../Components/TransformComponent.h"

static float CameraFollowSystem_Lerp(float current, float target, float alpha)
{
    return current + ((target - current) * alpha);
}

void CameraFollowSystem_Update(struct Scene* scene, float dt_seconds)
{
    size_t index = 0;

    if (scene == NULL || scene->is_overlay)
    {
        return;
    }

    for (index = 0; index < scene->entity_count; ++index)
    {
        Entity* entity = scene->entities[index];
        CameraTargetComponent* camera_target = NULL;
        TransformComponent* transform = NULL;
        float smoothing = 0.0f;
        float target_x = 0.0f;
        float target_y = 0.0f;
        float max_x = 0.0f;
        float max_y = 0.0f;

        if (entity == NULL || !entity->active)
        {
            continue;
        }

        camera_target = (CameraTargetComponent*)Entity_GetComponent(entity, COMPONENT_CAMERA_TARGET);
        transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
        if (camera_target == NULL || transform == NULL || !camera_target->follow)
        {
            continue;
        }

        smoothing = camera_target->smoothing > 0.0f ? camera_target->smoothing : scene->view.smoothing;
        target_x = transform->x + camera_target->lead_x - (scene->view.view_width * 0.5f);
        target_y = transform->y + camera_target->lead_y - (scene->view.view_height * 0.5f);
        scene->view.previous_x = scene->view.x;
        scene->view.previous_y = scene->view.y;

        if (smoothing > 0.0f)
        {
            float alpha = dt_seconds / (dt_seconds + smoothing);
            scene->view.x = CameraFollowSystem_Lerp(scene->view.x, target_x, alpha);
            scene->view.y = CameraFollowSystem_Lerp(scene->view.y, target_y, alpha);
        }
        else
        {
            scene->view.x = target_x;
            scene->view.y = target_y;
        }

        max_x = scene->view.world_max_x - scene->view.view_width;
        max_y = scene->view.world_max_y - scene->view.view_height;

        if (scene->view.has_world_bounds)
        {
            if (scene->view.x < scene->view.world_min_x)
            {
                scene->view.x = scene->view.world_min_x;
            }
            if (scene->view.y < scene->view.world_min_y)
            {
                scene->view.y = scene->view.world_min_y;
            }
            if (scene->view.x > max_x)
            {
                scene->view.x = max_x;
            }
            if (scene->view.y > max_y)
            {
                scene->view.y = max_y;
            }
        }
        return;
    }
}
