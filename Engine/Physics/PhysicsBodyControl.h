#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSBODYCONTROL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSBODYCONTROL_H

#include "PhysicsWorld.h"

void PhysicsWorld_RemoveBodyForEntity(PhysicsWorld* world, struct Entity* entity);
void PhysicsWorld_ClearEntityBodies(PhysicsWorld* world, struct Scene* scene);
void PhysicsWorld_SetEntityPosition(PhysicsWorld* world, struct Entity* entity, float x, float y);
bool PhysicsWorld_GetEntityLinearVelocity(PhysicsWorld* world, struct Entity* entity, Vec2* out_velocity);
bool PhysicsWorld_SetEntityLinearVelocity(PhysicsWorld* world, struct Entity* entity, Vec2 velocity);
bool PhysicsWorld_SetEntityAngularVelocity(PhysicsWorld* world, struct Entity* entity, float angular_velocity);
bool PhysicsWorld_ApplyEntityForce(PhysicsWorld* world, struct Entity* entity, Vec2 force, bool wake);
bool PhysicsWorld_ApplyEntityLinearImpulse(PhysicsWorld* world, struct Entity* entity, Vec2 impulse, bool wake);
bool PhysicsWorld_SetEntityAwake(PhysicsWorld* world, struct Entity* entity, bool awake);
bool PhysicsWorld_IsEntityAwake(PhysicsWorld* world, struct Entity* entity, bool* out_awake);
bool PhysicsWorld_GetEntityContactCapacity(PhysicsWorld* world, struct Entity* entity, int* out_contact_capacity);

#endif
