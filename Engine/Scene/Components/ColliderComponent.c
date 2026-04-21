#include "ColliderComponent.h"

#include "../Entity.h"

#include <math.h>
#include <stdlib.h>

static void ColliderComponent_RebuildBounds(ColliderComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    if (component->shape == COLLIDER_SHAPE_RECT)
    {
        component->bounds.min_x = -component->width * 0.5f;
        component->bounds.min_y = -component->height * 0.5f;
        component->bounds.max_x = component->width * 0.5f;
        component->bounds.max_y = component->height * 0.5f;
    }
    else
    {
        component->bounds.min_x = -component->radius;
        component->bounds.min_y = -component->radius;
        component->bounds.max_x = component->radius;
        component->bounds.max_y = component->radius;
    }

    component->dirty_flags |= COLLIDER_DIRTY_BOUNDS;
    if (component->base.entity != NULL)
    {
        Entity_MarkDirty(component->base.entity, ENTITY_DIRTY_VISIBILITY);
    }
}

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
    component->dirty_flags = COLLIDER_DIRTY_SHAPE | COLLIDER_DIRTY_BOUNDS;
    component->is_sensor = false;
    ColliderComponent_RebuildBounds(component);
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

void ColliderComponent_SetCircle(ColliderComponent* component, float radius)
{
    if (component == NULL)
    {
        return;
    }

    if (component->shape == COLLIDER_SHAPE_CIRCLE && fabsf(component->radius - radius) <= 0.0001f)
    {
        return;
    }

    component->shape = COLLIDER_SHAPE_CIRCLE;
    component->radius = radius;
    component->width = radius * 2.0f;
    component->height = radius * 2.0f;
    component->dirty_flags |= COLLIDER_DIRTY_SHAPE;
    ColliderComponent_RebuildBounds(component);
    if (component->base.entity != NULL)
    {
        Entity_MarkDirty(component->base.entity, ENTITY_DIRTY_VISIBILITY);
    }
}

void ColliderComponent_SetRect(ColliderComponent* component, float width, float height)
{
    if (component == NULL)
    {
        return;
    }

    if (component->shape == COLLIDER_SHAPE_RECT &&
        fabsf(component->width - width) <= 0.0001f &&
        fabsf(component->height - height) <= 0.0001f)
    {
        return;
    }

    component->shape = COLLIDER_SHAPE_RECT;
    component->width = width;
    component->height = height;
    component->radius = width > height ? width * 0.5f : height * 0.5f;
    component->dirty_flags |= COLLIDER_DIRTY_SHAPE;
    ColliderComponent_RebuildBounds(component);
    if (component->base.entity != NULL)
    {
        Entity_MarkDirty(component->base.entity, ENTITY_DIRTY_VISIBILITY);
    }
}

void ColliderComponent_SetSensor(ColliderComponent* component, bool is_sensor)
{
    if (component == NULL || component->is_sensor == is_sensor)
    {
        return;
    }

    component->is_sensor = is_sensor;
    component->dirty_flags |= COLLIDER_DIRTY_SHAPE;
    if (component->base.entity != NULL)
    {
        Entity_MarkDirty(component->base.entity, ENTITY_DIRTY_VISIBILITY);
    }
}

void ColliderComponent_ClearDirty(ColliderComponent* component, uint32_t dirty_flags)
{
    if (component == NULL)
    {
        return;
    }

    component->dirty_flags &= ~dirty_flags;
}
