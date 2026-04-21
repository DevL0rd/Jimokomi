#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_SCRATCH_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_SCRATCH_INTERNAL_H

#include "RendererInternal.h"

struct RendererScratchState {
    SpriteRenderable* scratch_items;
    size_t scratch_capacity;
    SurfaceDrawInstance* scratch_instances;
    size_t scratch_instance_capacity;
};

#endif
