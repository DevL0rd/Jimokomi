#ifndef JIMOKOMI_ENGINE_RENDERING_GRIDBACKDROP_H
#define JIMOKOMI_ENGINE_RENDERING_GRIDBACKDROP_H

#include "Camera.h"
#include "Target.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct GridBackdropConfig
{
    float world_width;
    float world_height;
    float cell_size;
    uint32_t even_color;
    uint32_t odd_color;
    uint32_t border_color;
    uint64_t dirty_version;
    bool dirty;
} GridBackdropConfig;

void grid_backdrop_init(GridBackdropConfig* backdrop, float world_width, float world_height, float cell_size);
void grid_backdrop_mark_dirty(GridBackdropConfig* backdrop);
void grid_backdrop_clear_dirty(GridBackdropConfig* backdrop);
uint64_t grid_backdrop_signature(const GridBackdropConfig* backdrop);
uint64_t grid_backdrop_signature_from_user_data(void* user_data);
void grid_backdrop_draw(Target* target, const Camera* camera, void* user_data);

#endif
