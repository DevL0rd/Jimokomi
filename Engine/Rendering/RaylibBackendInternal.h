#ifndef JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_INTERNAL_H

#include "RaylibBackend.h"

typedef struct RaylibBackend {
    RenderBackend render_backend;
    int window_width;
    int window_height;
    int target_width;
    int target_height;
    int clip_depth;
    Rect clip_stack[16];
    EngineInputSnapshot input_snapshot;
    bool should_close;
    bool frame_active;
    bool target_active;
    bool instancing_enabled;
    void* instancing_state;
} RaylibBackend;

bool raylib_backend_texture_get_dimensions(
    const Texture *texture,
    int *width,
    int *height
);
unsigned int raylib_backend_texture_get_texture_id(const Texture *texture);
void raylib_backend_draw_texture_batch_individual(
    void *userdata,
    const Texture *texture,
    const TextureDrawInstance *instances,
    size_t instance_count
);

void raylib_backend_draw_texture_batch(
    void *userdata,
    const Texture *texture,
    const TextureDrawInstance *instances,
    size_t instance_count
);
void raylib_backend_release_instancing_state(RaylibBackend* backend);

#endif
