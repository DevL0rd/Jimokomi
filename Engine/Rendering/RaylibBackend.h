#ifndef JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_H
#define JIMOKOMI_ENGINE_RENDERING_RAYLIBBACKEND_H

#include "Target.h"
#include "../Core/Input.h"

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
} RaylibBackend;

bool raylib_backend_init(
    RaylibBackend *backend,
    int width,
    int height,
    const char *title
);
void raylib_backend_dispose(RaylibBackend *backend);
void raylib_backend_pump_events(RaylibBackend *backend);
void raylib_backend_begin_frame(RaylibBackend *backend, Color32 clear_color);
void raylib_backend_end_frame(RaylibBackend *backend);
bool raylib_backend_should_close(const RaylibBackend *backend);
uint64_t raylib_backend_now_ms(void);
EngineInputSnapshot raylib_backend_snapshot_input(const RaylibBackend *backend);

#endif
