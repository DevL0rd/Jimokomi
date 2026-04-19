#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_TRANSFORMCOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_TRANSFORMCOMPONENT_H

#include "../Component.h"

typedef struct TransformComponent
{
    Component base;
    float x;
    float y;
    float angle_radians;
    float scale_x;
    float scale_y;
    bool dirty;
} TransformComponent;

void TransformComponent_Init(TransformComponent* component, float x, float y, float angle_radians);
TransformComponent* TransformComponent_Create(float x, float y, float angle_radians);
void TransformComponent_Destroy(TransformComponent* component);

#endif
