#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INVALIDATION_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INVALIDATION_INTERNAL_H

#include "ResourceManagerInternal.h"

struct ResourceInvalidationState {
    ResourceHandle* dirty_visual_source_handles;
    size_t dirty_visual_source_count;
    size_t dirty_visual_source_capacity;
    ResourceHandle* dirty_material_handles;
    size_t dirty_material_count;
    size_t dirty_material_capacity;
    ResourceHandle* dirty_shader_handles;
    size_t dirty_shader_count;
    size_t dirty_shader_capacity;
    BakedSurfaceKey* dirty_baked_surface_keys;
    size_t dirty_baked_surface_count;
    size_t dirty_baked_surface_capacity;
};

void resource_manager_mark_dirty_visual_source(ResourceManager* manager, ResourceHandle handle);
void resource_manager_mark_dirty_material(ResourceManager* manager, ResourceHandle handle);
void resource_manager_mark_dirty_shader(ResourceManager* manager, ResourceHandle handle);

#endif
