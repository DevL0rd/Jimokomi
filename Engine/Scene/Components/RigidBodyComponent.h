#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_RIGIDBODYCOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_RIGIDBODYCOMPONENT_H

#include "../Component.h"

#include <box2d/box2d.h>

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
    b2BodyId body_id;
    b2ShapeId shape_id;
    bool has_body;
} RigidBodyComponent;

void RigidBodyComponent_Init(RigidBodyComponent* component);
RigidBodyComponent* RigidBodyComponent_Create(void);
void RigidBodyComponent_Destroy(RigidBodyComponent* component);

b2BodyType RigidBodyComponent_ToBox2DType(RigidBodyType type);

#endif
