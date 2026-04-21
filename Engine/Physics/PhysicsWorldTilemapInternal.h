#ifndef JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_TILEMAP_INTERNAL_H
#define JIMOKOMI_ENGINE_PHYSICS_PHYSICSWORLD_TILEMAP_INTERNAL_H

#include "PhysicsWorldInternal.h"

struct PhysicsWorldTilemapState {
    const struct SceneTilemapAdapter* tilemap_adapter;
    const void* tilemap;
    const struct TileRule* tile_rules;
    size_t tile_rule_count;
    b2BodyId* tile_bodies;
    size_t tile_body_count;
    size_t tile_body_capacity;
};

#endif
