#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_LIFECYCLE_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_LIFECYCLE_INTERNAL_H

#include "RendererInternal.h"

struct RendererLifecycleState {
    DebugOverlay* debug_overlay;
    Camera camera;
    ResourceManager* resource_manager;
    int view_width;
    int view_height;
    float prebake_target_fps;
};

#endif
