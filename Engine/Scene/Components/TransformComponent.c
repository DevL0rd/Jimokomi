#include "TransformComponent.h"

#include <stdlib.h>

static void TransformComponent_DestroyBase(Component* component)
{
    TransformComponent_Destroy((TransformComponent*)component);
}

void TransformComponent_Init(TransformComponent* component, float x, float y, float angle_radians)
{
    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_TRANSFORM, TransformComponent_DestroyBase);
    component->x = x;
    component->y = y;
    component->angle_radians = angle_radians;
    component->scale_x = 1.0f;
    component->scale_y = 1.0f;
    component->dirty = false;
}

TransformComponent* TransformComponent_Create(float x, float y, float angle_radians)
{
    TransformComponent* component = (TransformComponent*)calloc(1, sizeof(TransformComponent));
    if (component == NULL)
    {
        return NULL;
    }

    TransformComponent_Init(component, x, y, angle_radians);
    return component;
}

void TransformComponent_Destroy(TransformComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}
