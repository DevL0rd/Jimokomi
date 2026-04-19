#ifndef JIMOKOMI_GAME_BALLVISUALS_H
#define JIMOKOMI_GAME_BALLVISUALS_H

#include "../Engine/Core/RuntimeConfig.h"
#include "../Engine/Rendering/Camera.h"
#include "../Engine/Rendering/Renderer.h"
#include "../Engine/Rendering/Target.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct WorldBackdropConfig {
    float world_width;
    float world_height;
    float cell_size;
} WorldBackdropConfig;

bool game_register_ball_visuals(
    Renderer* renderer,
    const RuntimeConfig* runtime_config,
    ResourceHandle* shared_shader_handle,
    ResourceHandle* source_handles,
    size_t source_count,
    ResourceHandle* material_handles,
    size_t material_count
);
size_t game_queue_required_ball_prebakes(
    Renderer* renderer,
    ResourceHandle shader_handle,
    const ResourceHandle* source_handles,
    size_t source_count,
    ResourceHandle representative_material_handle
);
void game_draw_world_backdrop(Target* target, const Camera* camera, void* user_data);

#endif
