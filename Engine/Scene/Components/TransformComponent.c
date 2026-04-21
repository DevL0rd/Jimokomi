#include "TransformComponent.h"

#include "../Entity.h"

#include <math.h>
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
    component->previous_x = x;
    component->previous_y = y;
    component->previous_angle_radians = angle_radians;
    component->x = x;
    component->y = y;
    component->angle_radians = angle_radians;
    component->scale_x = 1.0f;
    component->scale_y = 1.0f;
    component->dirty_flags = TRANSFORM_DIRTY_NONE;
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

void TransformComponent_MarkDirty(TransformComponent* component, uint32_t dirty_flags)
{
    if (component == NULL)
    {
        return;
    }

    component->dirty_flags |= dirty_flags;
    component->dirty = component->dirty_flags != TRANSFORM_DIRTY_NONE;
    if (component->base.entity != NULL)
    {
        Entity_MarkDirty(component->base.entity, ENTITY_DIRTY_VISIBILITY);
    }
}

void TransformComponent_ClearDirty(TransformComponent* component, uint32_t dirty_flags)
{
    if (component == NULL)
    {
        return;
    }

    component->dirty_flags &= ~dirty_flags;
    component->dirty = component->dirty_flags != TRANSFORM_DIRTY_NONE;
}

void TransformComponent_SetPosition(TransformComponent* component, float x, float y, bool teleported)
{
    if (component == NULL)
    {
        return;
    }

    if (fabsf(component->x - x) <= 0.0001f && fabsf(component->y - y) <= 0.0001f)
    {
        return;
    }

    component->x = x;
    component->y = y;
    TransformComponent_MarkDirty(
        component,
        TRANSFORM_DIRTY_POSITION | (teleported ? TRANSFORM_DIRTY_TELEPORT : TRANSFORM_DIRTY_NONE)
    );
}

void TransformComponent_SetRotation(TransformComponent* component, float angle_radians)
{
    if (component == NULL || fabsf(component->angle_radians - angle_radians) <= 0.0001f)
    {
        return;
    }

    component->angle_radians = angle_radians;
    TransformComponent_MarkDirty(component, TRANSFORM_DIRTY_ROTATION);
}

void TransformComponent_SetScale(TransformComponent* component, float scale_x, float scale_y)
{
    if (component == NULL)
    {
        return;
    }

    if (fabsf(component->scale_x - scale_x) <= 0.0001f &&
        fabsf(component->scale_y - scale_y) <= 0.0001f)
    {
        return;
    }

    component->scale_x = scale_x;
    component->scale_y = scale_y;
    TransformComponent_MarkDirty(component, TRANSFORM_DIRTY_SCALE);
}
