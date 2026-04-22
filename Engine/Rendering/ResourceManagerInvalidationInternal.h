#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INVALIDATION_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INVALIDATION_INTERNAL_H

#include "ResourceManagerInternal.h"

struct ResourceInvalidationState {
    ResourceHandle* dirty_procedural_texture_handles;
    size_t dirty_procedural_texture_count;
    size_t dirty_procedural_texture_capacity;
    ResourceHandle* dirty_material_handles;
    size_t dirty_material_count;
    size_t dirty_material_capacity;
    ResourceHandle* dirty_shader_handles;
    size_t dirty_shader_count;
    size_t dirty_shader_capacity;
    BakedTextureKey* dirty_baked_texture_keys;
    size_t dirty_baked_texture_count;
    size_t dirty_baked_texture_capacity;
};

void resource_manager_mark_dirty_procedural_texture(ResourceManager* manager, ResourceHandle handle);
void resource_manager_mark_dirty_material(ResourceManager* manager, ResourceHandle handle);
void resource_manager_mark_dirty_shader(ResourceManager* manager, ResourceHandle handle);

#endif
