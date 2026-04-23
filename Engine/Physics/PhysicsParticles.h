#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICS_PARTICLES_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICS_PARTICLES_H

#include "PhysicsHandles.h"
#include "PhysicsWorld.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum PhysicsParticleFlag {
    PHYSICS_PARTICLE_FLAG_LIQUID = 0U,
    PHYSICS_PARTICLE_FLAG_VISCOUS = 1U << 5,
    PHYSICS_PARTICLE_FLAG_TENSILE = 1U << 7,
    PHYSICS_PARTICLE_FLAG_COLOR_MIXING = 1U << 8,
    PHYSICS_PARTICLE_FLAG_STATIC_PRESSURE = 1U << 11
} PhysicsParticleFlag;

typedef struct PhysicsParticleSystemDesc {
    float radius;
    float density;
    float gravity_scale;
    float damping_strength;
    float pressure_strength;
    float viscous_strength;
    float surface_tension_pressure_strength;
    float surface_tension_normal_strength;
    int max_count;
    int iteration_count;
    bool destroy_by_age;
    float lifetime_granularity;
} PhysicsParticleSystemDesc;

typedef struct PhysicsParticleDesc {
    uint32_t flags;
    Vec2 position;
    Vec2 velocity;
    uint32_t color_argb;
    float lifetime_seconds;
    uintptr_t user_tag;
} PhysicsParticleDesc;

typedef struct PhysicsParticleRenderData {
    Vec2 previous_position;
    Vec2 position;
    float radius;
    uint32_t color_argb;
    uintptr_t user_tag;
} PhysicsParticleRenderData;

bool PhysicsWorld_CreateParticleSystem(
    PhysicsWorld* world,
    const PhysicsParticleSystemDesc* desc,
    PhysicsParticleSystemHandle* out_handle
);
void PhysicsWorld_DestroyParticleSystem(PhysicsWorld* world, PhysicsParticleSystemHandle handle);
bool PhysicsWorld_IsParticleSystemValid(const PhysicsWorld* world, PhysicsParticleSystemHandle handle);
bool PhysicsWorld_CreateParticle(
    PhysicsWorld* world,
    PhysicsParticleSystemHandle handle,
    const PhysicsParticleDesc* desc,
    int* out_particle_index
);
size_t PhysicsWorld_GetParticleCount(const PhysicsWorld* world);
size_t PhysicsWorld_GetParticleSystemParticleCount(const PhysicsWorld* world, PhysicsParticleSystemHandle handle);
float PhysicsWorld_GetParticleSystemRadius(const PhysicsWorld* world, PhysicsParticleSystemHandle handle);
size_t PhysicsWorld_CopyParticleRenderData(
    const PhysicsWorld* world,
    PhysicsParticleRenderData* particles,
    size_t capacity
);
size_t PhysicsWorld_CopyParticleSystemRenderData(
    const PhysicsWorld* world,
    PhysicsParticleSystemHandle handle,
    PhysicsParticleRenderData* particles,
    size_t capacity
);
size_t PhysicsWorld_CopyParticleSystemRenderDataInAabb(
    const PhysicsWorld* world,
    PhysicsParticleSystemHandle handle,
    Aabb bounds,
    PhysicsParticleRenderData* particles,
    size_t capacity
);

#endif
