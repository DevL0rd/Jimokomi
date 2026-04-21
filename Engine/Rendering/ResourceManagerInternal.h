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

typedef struct BakedSurfaceKey {
    uint32_t visual_source_id;
    uint32_t material_id;
    uint32_t shader_id;
    uint32_t frame_index;
    uint8_t pass;
} BakedSurfaceKey;

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
void resource_manager_mark_dirty_baked_surface(ResourceManager* manager, BakedSurfaceKey key);
uint64_t resource_manager_hash_baked_key(BakedSurfaceKey key);
bool resource_manager_baked_key_equals(BakedSurfaceKey a, BakedSurfaceKey b);
const Surface* resource_manager_find_baked_surface(const ResourceManager* manager, BakedSurfaceKey key);
bool resource_manager_store_baked_surface(ResourceManager* manager, BakedSurfaceKey key, Surface* surface);
uint32_t resource_manager_normalize_frame_index(const VisualSourceResource* source, uint32_t frame_index);
bool resource_manager_is_bake_eligible(const VisualSourceResource* source, BakedSurfacePass pass);
uint32_t resource_manager_get_bake_frame_count(const VisualSourceResource* source);
bool resource_manager_enqueue_baked_request(ResourceManager* manager, BakedSurfaceKey key, bool bypass_admission);
void resource_manager_remove_pending_bake_slot(ResourceManager* manager, BakedSurfaceKey key);
void resource_manager_reset_empty_bake_queue(ResourceManager* manager);

#endif
