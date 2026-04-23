#ifndef JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_INTERNAL_H

#include "RaylibBackend.h"

#include <stdint.h>

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
    bool tracy_frame_images_enabled;
    bool tracy_frame_images_ready;
    int tracy_frame_image_width;
    int tracy_frame_image_height;
    unsigned int tracy_frame_image_pbos[3];
    uintptr_t tracy_frame_image_fences[3];
    unsigned int tracy_frame_image_write_index;
    unsigned int tracy_frame_image_pending_count;
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
