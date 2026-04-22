#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_CACHE_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_CACHE_INTERNAL_H

#include "ResourceManagerInternal.h"

typedef struct BakedTextureResource {
    BakedTextureKey key;
    Texture* texture;
    uint32_t source_frame_index;
} BakedTextureResource;

typedef struct BakedTextureSlot {
    uint8_t state;
    BakedTextureKey key;
    size_t index;
} BakedTextureSlot;

typedef struct BakedMeshResource {
    ResourceHandle procedural_mesh_handle;
    uint32_t frame_index;
    uint32_t source_frame_index;
    Mesh* mesh;
} BakedMeshResource;

struct ResourceBakeCacheState {
    BakedTextureResource* baked_textures;
    size_t baked_texture_count;
    size_t baked_texture_capacity;
    BakedTextureSlot* baked_texture_slots;
    size_t baked_texture_slot_capacity;
    size_t baked_texture_slot_count;
    BakedMeshResource* baked_meshes;
    size_t baked_mesh_count;
    size_t baked_mesh_capacity;
};

bool resource_manager_reserve_baked_textures(ResourceManager* manager, size_t required);
bool resource_manager_insert_baked_texture_slot(ResourceManager* manager, BakedTextureKey key, size_t value_index);

#endif
