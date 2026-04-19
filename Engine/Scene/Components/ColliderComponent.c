#include "ColliderComponent.h"

#include <stdlib.h>

static void ColliderComponent_DestroyBase(Component* component)
{
    ColliderComponent_Destroy((ColliderComponent*)component);
}

void ColliderComponent_Init(ColliderComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_COLLIDER, ColliderComponent_DestroyBase);
    component->shape = COLLIDER_SHAPE_CIRCLE;
    component->radius = 8.0f;
    component->width = 16.0f;
    component->height = 16.0f;
    component->is_sensor = false;
}

ColliderComponent* ColliderComponent_Create(void)
{
    ColliderComponent* component = (ColliderComponent*)calloc(1, sizeof(ColliderComponent));
    if (component == NULL)
    {
        return NULL;
    }

    ColliderComponent_Init(component);
    return component;
}

void ColliderComponent_Destroy(ColliderComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}
