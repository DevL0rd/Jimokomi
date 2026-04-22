#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_INTERNAL_H

#include "PhysicsWorld.h"
#include "PhysicsHandles.h"

#include <corephys/corephys.h>

typedef struct PhysicsWorldLifecycleState PhysicsWorldLifecycleState;
typedef struct PhysicsWorldTilemapState PhysicsWorldTilemapState;
typedef struct PhysicsWorldEntityState PhysicsWorldEntityState;
typedef struct PhysicsWorldStatsState PhysicsWorldStatsState;
typedef struct PhysicsWorldParticleState PhysicsWorldParticleState;

struct PhysicsWorld
{
    PhysicsWorldLifecycleState* lifecycle;
    PhysicsWorldTilemapState* tilemap;
    PhysicsWorldEntityState* entities;
    PhysicsWorldStatsState* stats;
    PhysicsWorldParticleState* particles;
};

b2BodyId PhysicsWorld_LoadBodyHandle(PhysicsBodyHandle handle);
PhysicsBodyHandle PhysicsWorld_StoreBodyHandle(b2BodyId body_id);
PhysicsShapeHandle PhysicsWorld_StoreShapeHandle(b2ShapeId shape_id);
b2BodyId PhysicsWorld_EnsureEntityBody(PhysicsWorld* world, struct Entity* entity);
bool PhysicsWorld_SetEntitySleepThreshold(PhysicsWorld* world, struct Entity* entity, float sleep_threshold, bool is_onscreen);
bool PhysicsWorld_GetEntityBody(
    PhysicsWorld* world,
    struct Entity* entity,
    bool create_if_missing,
    b2BodyId* out_body_id
);
struct Entity* PhysicsWorld_GetEntityForBody(PhysicsWorld* world, b2BodyId body_id);
void PhysicsWorld_ClearTilemapBodies(PhysicsWorld* world);

#endif
