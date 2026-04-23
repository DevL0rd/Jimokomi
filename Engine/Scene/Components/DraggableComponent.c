#include "DraggableComponent.h"

#include "../../Core/Memory.h"

#include <stdlib.h>

static void DraggableComponent_DestroyBase(Component* component)
{
    DraggableComponent_Destroy((DraggableComponent*)component);
}

void DraggableComponent_Init(DraggableComponent* component, float pick_radius)
{
    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_DRAGGABLE, DraggableComponent_DestroyBase);
    component->enabled = true;
    component->pick_radius = pick_radius > 0.0f ? pick_radius : 1.0f;
    component->release_velocity_scale = 0.55f;
}

DraggableComponent* DraggableComponent_Create(float pick_radius)
{
    DraggableComponent* component = (DraggableComponent*)calloc(1, sizeof(*component));
    if (component == NULL)
    {
        return NULL;
    }

    DraggableComponent_Init(component, pick_radius);
    return component;
}

DraggableComponent* DraggableComponent_CreateWithDesc(const DraggableComponentDesc* desc)
{
    DraggableComponent* component = DraggableComponent_Create(desc != NULL ? desc->pick_radius : 1.0f);
    if (component == NULL || desc == NULL)
    {
        return component;
    }

    component->enabled = desc->enabled;
    component->release_velocity_scale =
        desc->release_velocity_scale > 0.0f ? desc->release_velocity_scale : component->release_velocity_scale;
    return component;
}

void DraggableComponent_Destroy(DraggableComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}
