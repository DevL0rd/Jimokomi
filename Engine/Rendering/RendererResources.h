#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERERRESOURCES_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERERRESOURCES_H

#include "Renderer.h"
#include "ResourceDescriptors.h"

ResourceHandle renderer_register_material(Renderer* renderer, const char* name, const Material* material);
ResourceHandle renderer_register_shader(Renderer* renderer, const char* name, ShaderStyle style);
ResourceHandle renderer_register_procedural_source(
    Renderer* renderer,
    const char* name,
    const ProceduralSourceDesc* desc
);

#endif
