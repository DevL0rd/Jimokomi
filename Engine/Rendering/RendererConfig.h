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

#endif
