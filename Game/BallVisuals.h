#ifndef JIMOKOMI_GAME_BALLVISUALS_H
#define JIMOKOMI_GAME_BALLVISUALS_H

#include "../Engine/Rendering/ResourceTypes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct WorldBackdropConfig {
    float world_width;
    float world_height;
    float cell_size;
    uint64_t dirty_version;
    bool dirty;
} WorldBackdropConfig;

typedef struct Camera Camera;
typedef struct Renderer Renderer;
typedef struct RuntimeConfig RuntimeConfig;
typedef struct Target Target;

bool game_register_ball_visuals(
    Renderer* renderer,
    const RuntimeConfig* runtime_config,
    ResourceHandle* shared_shader_handle,
    ResourceHandle* source_handles,
    size_t source_count,
    ResourceHandle* material_handles,
    size_t material_count
);
void game_draw_world_backdrop(Target* target, const Camera* camera, void* user_data);

#endif
