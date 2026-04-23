#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INTERNAL_H

#include "ResourceDescriptors.h"
#include "ResourceManager.h"
#include "ResourceManagerBake.h"
#include "ResourceManagerRegistry.h"
#include "ResourceManagerStats.h"

enum {
    SLOT_STATE_EMPTY = 0,
    SLOT_STATE_FILLED = 1,
    SLOT_STATE_TOMBSTONE = 2
};

typedef struct BakedTextureKey {
    uint32_t procedural_texture_id;
    uint32_t material_id;
    uint32_t shader_id;
    uint32_t frame_index;
    uint8_t pass;
} BakedTextureKey;

typedef struct ResourceRegistryState ResourceRegistryState;
typedef struct ResourceBakeCacheState ResourceBakeCacheState;
typedef struct ResourceBakeQueueState ResourceBakeQueueState;
typedef struct ResourceInvalidationState ResourceInvalidationState;
typedef struct ResourceStatsState ResourceStatsState;

typedef struct ResourceManager {
    ResourceRegistryState* registry;
    ResourceBakeCacheState* bake_cache;
    ResourceBakeQueueState* bake_queue;
    ResourceInvalidationState* invalidation;
    ResourceStatsState* stats;
    RenderBackend* backend;
} ResourceManager;

ResourceHandle resource_handle(uint32_t id);
void resource_manager_mark_dirty_baked_texture(ResourceManager* manager, BakedTextureKey key);
uint64_t resource_manager_hash_baked_key(BakedTextureKey key);
bool resource_manager_baked_key_equals(BakedTextureKey a, BakedTextureKey b);
const Texture* resource_manager_find_baked_texture(const ResourceManager* manager, BakedTextureKey key);
Texture* resource_manager_find_mutable_baked_texture(ResourceManager* manager, BakedTextureKey key);
const Texture* resource_manager_find_baked_texture_for_source_frame(
    const ResourceManager* manager,
    BakedTextureKey key,
    uint32_t source_frame_index
);
bool resource_manager_store_baked_texture(
    ResourceManager* manager,
    BakedTextureKey key,
    Texture* texture,
    uint32_t source_frame_index
);
const Mesh* resource_manager_get_or_create_baked_mesh(
    ResourceManager* manager,
    ResourceHandle procedural_mesh_handle,
    uint64_t instance_signature,
    uint32_t frame_index,
    void* user_data
);
uint32_t resource_manager_normalize_frame_index(const ProceduralTextureResource* source, uint32_t frame_index);
bool resource_manager_is_bake_eligible(const ProceduralTextureResource* source, BakedTexturePass pass);
uint32_t resource_manager_get_bake_frame_count(const ProceduralTextureResource* source);
bool resource_manager_enqueue_baked_request(ResourceManager* manager, BakedTextureKey key, bool bypass_admission);
void resource_manager_remove_pending_bake_slot(ResourceManager* manager, BakedTextureKey key);
void resource_manager_reset_empty_bake_queue(ResourceManager* manager);

#endif
