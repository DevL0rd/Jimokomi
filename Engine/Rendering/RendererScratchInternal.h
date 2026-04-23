#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_SCRATCH_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_SCRATCH_INTERNAL_H

#include "RendererInternal.h"

struct RendererScratchState {
    MaterialRenderable* scratch_material_renderables;
    size_t scratch_capacity;
    TextureDrawInstance* scratch_instances;
    size_t scratch_instance_capacity;
    TriangleDrawInstance* scratch_triangles;
    size_t scratch_triangle_capacity;
};

#endif
