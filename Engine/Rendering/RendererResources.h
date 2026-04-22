#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERERRESOURCES_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERERRESOURCES_H

#include "Renderer.h"
#include "ResourceDescriptors.h"

typedef void (*RendererTextureBuildFn)(Target* target, void* user_data);

ResourceHandle renderer_register_texture_from_builder(
    Renderer* renderer,
    const char* name,
    int width,
    int height,
    RendererTextureBuildFn build_texture,
    void* user_data
);
ResourceHandle renderer_register_material(Renderer* renderer, const char* name, const Material* material);
ResourceHandle renderer_register_shader(Renderer* renderer, const char* name, ShaderStyle style);
ResourceHandle renderer_register_procedural_texture(
    Renderer* renderer,
    const char* name,
    const ProceduralTextureDesc* desc
);
ResourceHandle renderer_register_procedural_mesh(
    Renderer* renderer,
    const char* name,
    const ProceduralMeshDesc* desc
);

#endif
