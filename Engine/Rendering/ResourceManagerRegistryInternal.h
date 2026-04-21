#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_REGISTRY_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_REGISTRY_INTERNAL_H

#include "ResourceManagerInternal.h"

typedef struct ResourceEntry {
    char* key;
    uint32_t id;
} ResourceEntry;

struct ResourceRegistryState {
    ResourceEntry* textures;
    TextureResource* texture_values;
    size_t texture_count;
    size_t texture_capacity;

    ResourceEntry* materials;
    MaterialResource* material_values;
    size_t material_count;
    size_t material_capacity;

    ResourceEntry* shaders;
    ShaderResource* shader_values;
    size_t shader_count;
    size_t shader_capacity;

    ResourceEntry* visual_sources;
    VisualSourceResource* visual_source_values;
    size_t visual_source_count;
    size_t visual_source_capacity;
};

bool resource_manager_reserve(
    ResourceEntry** entries,
    void** values,
    size_t element_size,
    size_t* capacity,
    size_t required
);

#endif
