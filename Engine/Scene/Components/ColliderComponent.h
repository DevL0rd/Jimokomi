#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_COLLIDERCOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_COLLIDERCOMPONENT_H

#include "../Component.h"
#include "../../Core/Geometry.h"

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
    Aabb bounds;
    uint32_t dirty_flags;
    bool is_sensor;
} ColliderComponent;

typedef enum ColliderDirtyFlags
{
    COLLIDER_DIRTY_NONE = 0,
    COLLIDER_DIRTY_SHAPE = 1 << 0,
    COLLIDER_DIRTY_BOUNDS = 1 << 1
} ColliderDirtyFlags;

void ColliderComponent_Init(ColliderComponent* component);
ColliderComponent* ColliderComponent_Create(void);
ColliderComponent* ColliderComponent_CreateCircle(float radius);
ColliderComponent* ColliderComponent_CreateRect(float width, float height);
void ColliderComponent_Destroy(ColliderComponent* component);
void ColliderComponent_SetCircle(ColliderComponent* component, float radius);
void ColliderComponent_SetRect(ColliderComponent* component, float width, float height);
void ColliderComponent_SetSensor(ColliderComponent* component, bool is_sensor);
void ColliderComponent_ClearDirty(ColliderComponent* component, uint32_t dirty_flags);

#endif
