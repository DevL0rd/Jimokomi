#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_COLLIDERCOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_COLLIDERCOMPONENT_H

#include "../Component.h"

typedef enum ColliderShape
{
    COLLIDER_SHAPE_CIRCLE = 0,
    COLLIDER_SHAPE_RECT
} ColliderShape;

typedef struct ColliderComponent
{
    Component base;
    ColliderShape shape;
    float radius;
    float width;
    float height;
    bool is_sensor;
} ColliderComponent;

void ColliderComponent_Init(ColliderComponent* component);
ColliderComponent* ColliderComponent_Create(void);
void ColliderComponent_Destroy(ColliderComponent* component);

#endif
