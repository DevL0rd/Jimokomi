#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_TILEMAP_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_TILEMAP_INTERNAL_H

#include "SceneInternal.h"

struct SceneTilemapState {
    const void* tilemap;
    const SceneTilemapAdapter* tilemap_adapter;
    const TileRule* tile_rules;
    size_t tile_rule_count;
};

#endif
