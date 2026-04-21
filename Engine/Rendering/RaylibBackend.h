#ifndef JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_H
#define JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_H

#include "Target.h"
#include "../Core/Input.h"

typedef struct RaylibBackend RaylibBackend;

bool raylib_backend_init(
    RaylibBackend *backend,
    int width,
    int height,
    const char *title,
    bool vsync_enabled
);
RaylibBackend* raylib_backend_create(int width, int height, const char* title, bool vsync_enabled);
void raylib_backend_dispose(RaylibBackend *backend);
void raylib_backend_destroy(RaylibBackend* backend);
void raylib_backend_pump_events(RaylibBackend *backend);
void raylib_backend_begin_frame(RaylibBackend *backend, Color32 clear_color);
void raylib_backend_end_frame(RaylibBackend *backend);
bool raylib_backend_should_close(const RaylibBackend *backend);
uint64_t raylib_backend_now_ms(void);
EngineInputSnapshot raylib_backend_snapshot_input(const RaylibBackend *backend);
RenderBackend* raylib_backend_get_render_backend(RaylibBackend* backend);
const RenderBackend* raylib_backend_get_render_backend_const(const RaylibBackend* backend);
void raylib_backend_get_window_size(const RaylibBackend* backend, int* out_width, int* out_height);

#endif
