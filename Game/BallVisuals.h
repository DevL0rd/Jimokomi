#ifndef JIMOKOMI_GAME_BALLVISUALS_H
#define JIMOKOMI_GAME_BALLVISUALS_H

#include "../Engine/Core/RuntimeConfig.h"
#include "../Engine/Rendering/Camera.h"
#include "../Engine/Rendering/Renderer.h"
#include "../Engine/Rendering/Target.h"

#include <stdbool.h>
#include <stddef.h>

bool game_register_ball_visuals(
    Renderer* renderer,
    const RuntimeConfig* runtime_config,
    ResourceHandle* shared_shader_handle,
    ResourceHandle* source_handles,
    size_t source_count,
    ResourceHandle* material_handles,
    size_t material_count
);

bool game_prewarm_ball_visuals(Renderer* renderer);
void game_draw_world_backdrop(Target* target, const Camera* camera, float world_width, float world_height);

#endif
