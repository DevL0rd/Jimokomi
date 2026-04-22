#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_PROCESS_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_PROCESS_INTERNAL_H

#include "ResourceManagerInternal.h"

const Texture* resource_manager_execute_baked_texture(
    ResourceManager* manager,
    const ProceduralTextureResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedTextureKey key,
    uint32_t source_frame_index,
    void* user_data
);

#endif
