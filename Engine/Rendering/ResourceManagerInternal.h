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

typedef struct BakedSurfaceResource {
    BakedSurfaceKey key;
    Surface* surface;
} BakedSurfaceResource;

typedef struct BakedSurfaceSlot {
    uint8_t state;
    BakedSurfaceKey key;
    size_t index;
} BakedSurfaceSlot;

typedef struct PendingBakeRequest {
    BakedSurfaceKey key;
    uint32_t priority;
} PendingBakeRequest;

typedef struct PendingBakeSlot {
    uint8_t state;
    BakedSurfaceKey key;
} PendingBakeSlot;

typedef struct BakeInterestEntry {
    BakedSurfaceKey key;
    uint32_t total_hits;
    uint32_t frame_hits;
    uint64_t last_seen_frame;
} BakeInterestEntry;

typedef struct ResourceEntry {
    char* key;
    uint32_t id;
} ResourceEntry;

typedef struct ResourceRegistryState {
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
} ResourceRegistryState;

typedef struct ResourceBakeCacheState {
    BakedSurfaceResource* baked_surfaces;
    size_t baked_surface_count;
    size_t baked_surface_capacity;
    BakedSurfaceSlot* baked_surface_slots;
    size_t baked_surface_slot_capacity;
    size_t baked_surface_slot_count;
} ResourceBakeCacheState;

typedef struct ResourceBakeQueueState {
    PendingBakeRequest* static_pending_bake_requests;
    size_t static_pending_bake_request_count;
    size_t static_pending_bake_request_capacity;
    size_t static_pending_bake_request_head;
    PendingBakeRequest* transient_pending_bake_requests;
    size_t transient_pending_bake_request_count;
    size_t transient_pending_bake_request_capacity;
    size_t transient_pending_bake_request_head;
    PendingBakeSlot* pending_bake_slots;
    size_t pending_bake_slot_capacity;
    size_t pending_bake_slot_count;
    BakeInterestEntry* bake_interest_entries;
    size_t bake_interest_count;
    size_t bake_interest_capacity;
    size_t bake_requests_this_frame;
    double bake_time_budget_ms;
    double last_bake_process_ms;
    size_t bake_admission_total_hits;
    size_t bake_admission_frame_hits;
    uint64_t frame_serial;
    uint32_t* visual_source_last_requested_frame_indices;
    size_t visual_source_last_requested_frame_capacity;
} ResourceBakeQueueState;

typedef struct ResourceInvalidationState {
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
} ResourceInvalidationState;

typedef struct ResourceStatsState {
    uint64_t bake_cache_hits;
    uint64_t bake_cache_misses;
    uint64_t bake_invalidation_visual_source_count;
    uint64_t bake_invalidation_material_count;
    uint64_t bake_invalidation_shader_count;
    uint64_t bake_invalidation_animation_frame_count;
    uint64_t prebake_ready_visual_source_count;
    uint64_t prebake_ready_material_count;
} ResourceStatsState;

typedef struct ResourceManager {
    ResourceRegistryState registry;
    ResourceBakeCacheState bake_cache;
    ResourceBakeQueueState bake_queue;
    ResourceInvalidationState invalidation;
    ResourceStatsState stats;

    RenderBackend* backend;
} ResourceManager;

ResourceHandle resource_handle(uint32_t id);

bool resource_manager_reserve(
    ResourceEntry** entries,
    void** values,
    size_t element_size,
    size_t* capacity,
    size_t required
);
void resource_manager_mark_dirty_visual_source(ResourceManager* manager, ResourceHandle handle);
void resource_manager_mark_dirty_material(ResourceManager* manager, ResourceHandle handle);
void resource_manager_mark_dirty_shader(ResourceManager* manager, ResourceHandle handle);
void resource_manager_mark_dirty_baked_surface(ResourceManager* manager, BakedSurfaceKey key);
uint64_t resource_manager_hash_baked_key(BakedSurfaceKey key);
bool resource_manager_baked_key_equals(BakedSurfaceKey a, BakedSurfaceKey b);
bool resource_manager_reserve_baked_surfaces(ResourceManager* manager, size_t required);
bool resource_manager_insert_baked_surface_slot(ResourceManager* manager, BakedSurfaceKey key, size_t value_index);
const Surface* resource_manager_find_baked_surface(const ResourceManager* manager, BakedSurfaceKey key);
bool resource_manager_store_baked_surface(ResourceManager* manager, BakedSurfaceKey key, Surface* surface);
uint32_t resource_manager_normalize_frame_index(const VisualSourceResource* source, uint32_t frame_index);
bool resource_manager_is_bake_eligible(const VisualSourceResource* source, BakedSurfacePass pass);
uint32_t resource_manager_get_bake_frame_count(const VisualSourceResource* source);
const Surface* resource_manager_execute_baked_surface(
    ResourceManager* manager,
    const VisualSourceResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedSurfaceKey key,
    void* user_data
);
bool resource_manager_enqueue_baked_request(ResourceManager* manager, BakedSurfaceKey key, bool bypass_admission);
void resource_manager_remove_pending_bake_slot(ResourceManager* manager, BakedSurfaceKey key);
void resource_manager_reset_empty_bake_queue(ResourceManager* manager);

#endif
