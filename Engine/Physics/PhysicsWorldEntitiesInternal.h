#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_ENTITIES_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_ENTITIES_INTERNAL_H

#include "PhysicsWorldInternal.h"

struct PhysicsWorldEntityState {
    struct Entity** tracked_entities;
    struct Entity** onscreen_sleep_entities;
    size_t tracked_entity_count;
    size_t tracked_entity_capacity;
    size_t onscreen_sleep_entity_count;
    size_t onscreen_sleep_entity_capacity;
    uint32_t sleep_visibility_stamp;
};

#endif
