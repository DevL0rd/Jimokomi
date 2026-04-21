#ifndef JIMOKOMI_GAME_BALLVISUALRESOURCES_H
#define JIMOKOMI_GAME_BALLVISUALRESOURCES_H

#include "../Engine/Rendering/ResourceTypes.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Renderer Renderer;
typedef struct RuntimeConfig RuntimeConfig;

bool game_register_ball_visuals(
    Renderer* renderer,
    const RuntimeConfig* runtime_config,
    ResourceHandle* shared_shader_handle,
    ResourceHandle* source_handles,
    size_t source_count,
    ResourceHandle* material_handles,
    size_t material_count
);

#endif
