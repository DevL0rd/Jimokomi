#ifndef JIMOKOMI_GAME_BALLVISUALRESOURCES_H
#define JIMOKOMI_GAME_BALLVISUALRESOURCES_H

#include "../Engine/Rendering/ResourceTypes.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Renderer Renderer;

bool game_register_ball_visuals(
    Renderer* renderer,
    ResourceHandle* material_handle
);

#endif
