#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_RIGIDBODYCOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_RIGIDBODYCOMPONENT_H

#include "../Component.h"
#include "../../Physics/PhysicsHandles.h"

typedef enum RigidBodyType
{
    RIGID_BODY_STATIC = 0,
    RIGID_BODY_KINEMATIC,
    RIGID_BODY_DYNAMIC
} RigidBodyType;

typedef struct RigidBodyComponent
{
    Component base;
    RigidBodyType body_type;
    bool fixed_rotation;
    float density;
    float friction_air;
    float friction;
    float restitution;
    float initial_velocity_x;
    float initial_velocity_y;
    float initial_angular_velocity;
    PhysicsBodyHandle body_id;
    PhysicsShapeHandle shape_id;
    uint32_t dirty_flags;
    bool has_body;
} RigidBodyComponent;

typedef enum RigidBodyDirtyFlags
{
    RIGID_BODY_DIRTY_NONE = 0,
    RIGID_BODY_DIRTY_DEFINITION = 1 << 0
} RigidBodyDirtyFlags;

void RigidBodyComponent_Init(RigidBodyComponent* component);
RigidBodyComponent* RigidBodyComponent_Create(void);
void RigidBodyComponent_Destroy(RigidBodyComponent* component);
void RigidBodyComponent_MarkDefinitionDirty(RigidBodyComponent* component);
void RigidBodyComponent_ClearDirty(RigidBodyComponent* component, uint32_t dirty_flags);

#endif
