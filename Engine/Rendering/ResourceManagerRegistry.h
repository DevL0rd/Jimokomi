#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERREGISTRY_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERREGISTRY_H

#include "ResourceDescriptors.h"
#include "ResourceManager.h"
#include "Target.h"

typedef void (*ResourceTextureBuildFn)(Target* target, void* user_data);

ResourceHandle resource_manager_register_texture_from_builder(
    ResourceManager* manager,
    const char* key,
    int width,
    int height,
    ResourceTextureBuildFn build_texture,
    void* user_data
);

ResourceHandle resource_manager_register_texture(
    ResourceManager* manager,
    const char* key,
    Texture* texture
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
ResourceHandle resource_manager_register_procedural_texture(
    ResourceManager* manager,
    const char* key,
    const ProceduralTextureDesc* desc
);
ResourceHandle resource_manager_register_procedural_mesh(
    ResourceManager* manager,
    const char* key,
    const ProceduralMeshDesc* desc
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
const ProceduralTextureResource* resource_manager_get_procedural_texture(
    const ResourceManager* manager,
    ResourceHandle handle
);
const ProceduralMeshResource* resource_manager_get_procedural_mesh(
    const ResourceManager* manager,
    ResourceHandle handle
);

#endif
