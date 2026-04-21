#include "RigidBodyComponent.h"

#include "../Entity.h"

#include <stdlib.h>

static void RigidBodyComponent_DestroyBase(Component* component)
{
    RigidBodyComponent_Destroy((RigidBodyComponent*)component);
}

void RigidBodyComponent_Init(RigidBodyComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_RIGID_BODY, RigidBodyComponent_DestroyBase);
    component->body_type = RIGID_BODY_DYNAMIC;
    component->fixed_rotation = false;
    component->density = 0.001f;
    component->friction_air = 0.01f;
    component->friction = 0.1f;
    component->restitution = 0.0f;
    component->initial_velocity_x = 0.0f;
    component->initial_velocity_y = 0.0f;
    component->initial_angular_velocity = 0.0f;
    component->body_id = (PhysicsBodyHandle){ 0 };
    component->shape_id = (PhysicsShapeHandle){ 0 };
    component->dirty_flags = RIGID_BODY_DIRTY_DEFINITION;
    component->has_body = false;
}

RigidBodyComponent* RigidBodyComponent_Create(void)
{
    RigidBodyComponent* component = (RigidBodyComponent*)calloc(1, sizeof(RigidBodyComponent));
    if (component == NULL)
    {
        return NULL;
    }

    RigidBodyComponent_Init(component);
    return component;
}

RigidBodyComponent* RigidBodyComponent_CreateWithDesc(const RigidBodyComponentDesc* desc)
{
    RigidBodyComponent* component = RigidBodyComponent_Create();
    if (component == NULL)
    {
        return NULL;
    }
    if (desc == NULL)
    {
        return component;
    }

    component->body_type = desc->body_type;
    component->fixed_rotation = desc->fixed_rotation;
    component->density = desc->density > 0.0f ? desc->density : component->density;
    component->friction_air = desc->friction_air;
    component->friction = desc->friction;
    component->restitution = desc->restitution;
    component->initial_velocity_x = desc->initial_velocity_x;
    component->initial_velocity_y = desc->initial_velocity_y;
    component->initial_angular_velocity = desc->initial_angular_velocity;
    RigidBodyComponent_MarkDefinitionDirty(component);
    return component;
}

void RigidBodyComponent_Destroy(RigidBodyComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}

void RigidBodyComponent_MarkDefinitionDirty(RigidBodyComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    component->dirty_flags |= RIGID_BODY_DIRTY_DEFINITION;
    if (component->base.entity != NULL)
    {
        Entity_MarkDirty(component->base.entity, ENTITY_DIRTY_VISIBILITY);
    }
}

void RigidBodyComponent_ClearDirty(RigidBodyComponent* component, uint32_t dirty_flags)
{
    if (component == NULL)
    {
        return;
    }

    component->dirty_flags &= ~dirty_flags;
}
