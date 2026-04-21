#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERREGISTRY_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERREGISTRY_H

#include "ResourceDescriptors.h"
#include "ResourceManager.h"

ResourceHandle resource_manager_register_texture(
    ResourceManager* manager,
    const char* key,
    Surface* surface
);
ResourceHandle resource_manager_register_material(
    ResourceManager* manager,
    const char* key,
    const Material* material
);
ResourceHandle resource_manager_register_shader(
    ResourceManager* manager,
    const char* key,
    ShaderStyle style
);
ResourceHandle resource_manager_register_procedural_source(
    ResourceManager* manager,
    const char* key,
    const ProceduralSourceDesc* desc
);

const TextureResource* resource_manager_get_texture(
    const ResourceManager* manager,
    ResourceHandle handle
);
const MaterialResource* resource_manager_get_material(
    const ResourceManager* manager,
    ResourceHandle handle
);
const ShaderResource* resource_manager_get_shader(
    const ResourceManager* manager,
    ResourceHandle handle
);
const VisualSourceResource* resource_manager_get_visual_source(
    const ResourceManager* manager,
    ResourceHandle handle
);

#endif
