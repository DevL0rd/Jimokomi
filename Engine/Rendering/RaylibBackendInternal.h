#ifndef JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_INTERNAL_H

#include "RaylibBackend.h"

#include <raylib.h>

typedef struct RaylibSurface {
    Surface base;
    RenderTexture2D target;
} RaylibSurface;

Color raylib_unpack_color(Color32 color);

void raylib_backend_draw_surface_batch(
    void *userdata,
    const Surface *surface,
    const SurfaceDrawInstance *instances,
    size_t instance_count
);
void raylib_backend_release_instancing_state(RaylibBackend* backend);

#endif
