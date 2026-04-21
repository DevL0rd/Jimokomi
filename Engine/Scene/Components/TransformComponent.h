#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_TRANSFORMCOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_TRANSFORMCOMPONENT_H

#include "../Component.h"

typedef struct TransformComponent
{
    Component base;
    float previous_x;
    float previous_y;
    float previous_angle_radians;
    float x;
    float y;
    float angle_radians;
    float scale_x;
    float scale_y;
    uint32_t dirty_flags;
    bool dirty;
} TransformComponent;

typedef enum TransformDirtyFlags
{
    TRANSFORM_DIRTY_NONE = 0,
    TRANSFORM_DIRTY_POSITION = 1 << 0,
    TRANSFORM_DIRTY_ROTATION = 1 << 1,
    TRANSFORM_DIRTY_SCALE = 1 << 2,
    TRANSFORM_DIRTY_TELEPORT = 1 << 3
} TransformDirtyFlags;

void TransformComponent_Init(TransformComponent* component, float x, float y, float angle_radians);
TransformComponent* TransformComponent_Create(float x, float y, float angle_radians);
void TransformComponent_Destroy(TransformComponent* component);
void TransformComponent_MarkDirty(TransformComponent* component, uint32_t dirty_flags);
void TransformComponent_ClearDirty(TransformComponent* component, uint32_t dirty_flags);
void TransformComponent_SetPosition(TransformComponent* component, float x, float y, bool teleported);
void TransformComponent_SetRotation(TransformComponent* component, float angle_radians);
void TransformComponent_SetScale(TransformComponent* component, float scale_x, float scale_y);

#endif
