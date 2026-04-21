#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_INTERNAL_H

#include "ResourceManager.h"

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

typedef struct ResourceManager {
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

    BakedSurfaceResource* baked_surfaces;
    size_t baked_surface_count;
    size_t baked_surface_capacity;
    BakedSurfaceSlot* baked_surface_slots;
    size_t baked_surface_slot_capacity;
    size_t baked_surface_slot_count;
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
    size_t bake_budget_per_frame;
    size_t bake_requests_this_frame;
    size_t bake_admission_total_hits;
    size_t bake_admission_frame_hits;
    uint64_t frame_serial;
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
    uint32_t* visual_source_last_requested_frame_indices;
    size_t visual_source_last_requested_frame_capacity;
    uint64_t bake_cache_hits;
    uint64_t bake_cache_misses;
    uint64_t bake_invalidation_visual_source_count;
    uint64_t bake_invalidation_material_count;
    uint64_t bake_invalidation_shader_count;
    uint64_t bake_invalidation_animation_frame_count;
    uint64_t prebake_ready_visual_source_count;
    uint64_t prebake_ready_material_count;

    RenderBackend* backend;
} ResourceManager;

#endif
