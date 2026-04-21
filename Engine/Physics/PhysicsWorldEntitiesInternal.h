#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_ENTITIES_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_ENTITIES_INTERNAL_H

#include "PhysicsWorldInternal.h"

struct PhysicsWorldEntityState {
    struct Entity** tracked_entities;
    size_t tracked_entity_count;
    size_t tracked_entity_capacity;
};

#endif
