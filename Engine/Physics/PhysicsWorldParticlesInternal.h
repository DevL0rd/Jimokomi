#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_PARTICLES_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_PARTICLES_INTERNAL_H

#include "PhysicsParticles.h"

typedef struct PhysicsWorldTrackedParticleSystem
{
    PhysicsParticleSystemHandle handle;
    Vec2* previous_positions;
    size_t previous_count;
    size_t previous_capacity;
} PhysicsWorldTrackedParticleSystem;

typedef struct PhysicsWorldParticleState {
    PhysicsWorldTrackedParticleSystem* systems;
    size_t system_count;
    size_t system_capacity;
} PhysicsWorldParticleState;

void PhysicsWorld_CapturePreviousParticlePositions(PhysicsWorld* world);
void PhysicsWorld_DestroyParticleState(PhysicsWorld* world);

#endif
