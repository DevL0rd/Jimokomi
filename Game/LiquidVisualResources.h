#ifndef JIMOKOMI_GAME_LIQUID_VISUAL_RESOURCES_H
#define JIMOKOMI_GAME_LIQUID_VISUAL_RESOURCES_H

#include "../Engine/Rendering/ResourceTypes.h"

#include <stdbool.h>

typedef struct Renderer Renderer;

typedef struct LiquidVisualResourceHandles
{
    ResourceHandle source_handle;
    ResourceHandle shader_handle;
    ResourceHandle material_handle;
} LiquidVisualResourceHandles;

bool game_register_liquid_visuals(Renderer* renderer, LiquidVisualResourceHandles* handles);

#endif
