#ifndef JIMOKOMI_GAME_STATE_H
#define JIMOKOMI_GAME_STATE_H

#include "GameConfig.h"
#include "LiquidSourceSystem.h"

#include "../Engine/Rendering/GridBackdrop.h"
#include "../Engine/Rendering/ResourceTypes.h"
#include "../Engine/Scene/Entity.h"
#include "../Engine/Scene/Scene.h"

#include <stddef.h>

typedef struct JimokomiGameState
{
    Scene* scene;
    Entity* player;
    Entity* wave_paddle;
    GridBackdropConfig backdrop;
    ResourceHandle shared_ball_shader_handle;
    ResourceHandle ball_procedural_texture_handles[SOURCE_VARIANT_COUNT];
    ResourceHandle ball_material_handle;
    size_t active_ball_count;
    size_t spawn_cursor;
    double spawn_accumulator_seconds;
    double wave_paddle_time_seconds;
    LiquidSourceSystemState liquid_sources;
} JimokomiGameState;

#endif
