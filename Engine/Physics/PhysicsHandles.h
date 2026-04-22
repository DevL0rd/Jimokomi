#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSHANDLES_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSHANDLES_H

#include <stdint.h>

typedef struct PhysicsBodyHandle {
    int32_t index1;
    uint16_t world0;
    uint16_t generation;
} PhysicsBodyHandle;

typedef struct PhysicsShapeHandle {
    int32_t index1;
    uint16_t world0;
    uint16_t generation;
} PhysicsShapeHandle;

typedef struct PhysicsParticleSystemHandle {
    int32_t index1;
    uint16_t world0;
    uint16_t generation;
} PhysicsParticleSystemHandle;

#endif
