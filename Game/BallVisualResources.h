#ifndef JIMOKOMI_GAME_BALLVISUALRESOURCES_H
#define JIMOKOMI_GAME_BALLVISUALRESOURCES_H

#include "../Engine/Rendering/ResourceTypes.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Renderer Renderer;

bool game_register_ball_visuals(
    Renderer* renderer,
    ResourceHandle* shared_shader_handle,
    ResourceHandle* procedural_texture_handles,
    size_t procedural_texture_count,
    ResourceHandle* material_handles,
    size_t material_count
);

#endif
