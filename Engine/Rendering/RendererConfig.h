#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERERCONFIG_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERERCONFIG_H

#include <stddef.h>

typedef struct RendererConfig {
    int view_width;
    int view_height;
    float prebake_target_fps;
    size_t prebake_admission_total_hits;
    size_t prebake_admission_frame_hits;
} RendererConfig;

static inline RendererConfig RendererConfig_Defaults(void) {
    RendererConfig config = {
        .view_width = 960,
        .view_height = 540,
        .prebake_target_fps = 60.0f,
        .prebake_admission_total_hits = 1U,
        .prebake_admission_frame_hits = 1U
    };
    return config;
}

#endif
