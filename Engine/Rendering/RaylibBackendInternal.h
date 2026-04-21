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

bool raylib_backend_surface_get_dimensions(
    const Surface *surface,
    int *width,
    int *height
);
unsigned int raylib_backend_surface_get_texture_id(const Surface *surface);
void raylib_backend_draw_surface_batch_individual(
    void *userdata,
    const Surface *surface,
    const SurfaceDrawInstance *instances,
    size_t instance_count
);

void raylib_backend_draw_surface_batch(
    void *userdata,
    const Surface *surface,
    const SurfaceDrawInstance *instances,
    size_t instance_count
);
void raylib_backend_release_instancing_state(RaylibBackend* backend);

#endif
