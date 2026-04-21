#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_PROCESS_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_PROCESS_INTERNAL_H

#include "ResourceManagerInternal.h"

const Surface* resource_manager_execute_baked_surface(
    ResourceManager* manager,
    const VisualSourceResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedSurfaceKey key,
    void* user_data
);

#endif
