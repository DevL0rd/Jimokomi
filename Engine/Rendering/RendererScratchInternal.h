#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_SCRATCH_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_SCRATCH_INTERNAL_H

#include "RendererInternal.h"

struct RendererScratchState {
    ProceduralTextureRenderable* scratch_procedural_textures;
    size_t scratch_capacity;
    TextureDrawInstance* scratch_instances;
    size_t scratch_instance_capacity;
};

#endif
