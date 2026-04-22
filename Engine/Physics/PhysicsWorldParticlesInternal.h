#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_PARTICLES_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_PARTICLES_INTERNAL_H

#include "PhysicsParticles.h"

typedef struct PhysicsWorldParticleState {
    PhysicsParticleSystemHandle* systems;
    size_t system_count;
    size_t system_capacity;
} PhysicsWorldParticleState;

void PhysicsWorld_DestroyParticleState(PhysicsWorld* world);

#endif
