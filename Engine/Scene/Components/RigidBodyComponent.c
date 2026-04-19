#include "RigidBodyComponent.h"

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
    component->body_id = b2_nullBodyId;
    component->shape_id = b2_nullShapeId;
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

void RigidBodyComponent_Destroy(RigidBodyComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}

b2BodyType RigidBodyComponent_ToBox2DType(RigidBodyType type)
{
    switch (type)
    {
        case RIGID_BODY_STATIC:
            return b2_staticBody;
        case RIGID_BODY_KINEMATIC:
            return b2_kinematicBody;
        case RIGID_BODY_DYNAMIC:
        default:
            return b2_dynamicBody;
    }
}
