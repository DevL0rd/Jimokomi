#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERSTATS_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGERSTATS_H

#include "ResourceManager.h"

#include <stddef.h>
#include <stdint.h>

typedef struct ResourceManagerStatsSnapshot {
    size_t texture_count;
    size_t material_count;
    size_t shader_count;
    size_t procedural_texture_count;
    size_t baked_texture_count;
    size_t pending_bake_count;
    size_t static_pending_bake_count;
    size_t transient_pending_bake_count;
    size_t dirty_procedural_texture_count;
    size_t dirty_material_count;
    size_t dirty_shader_count;
    size_t dirty_baked_texture_count;
    size_t bake_interest_count;
    size_t bake_requests_this_frame;
    double bake_time_budget_ms;
    double bake_process_ms;
    size_t baked_texture_memory_bytes;
    uint64_t bake_cache_hits;
    uint64_t bake_cache_misses;
    uint64_t bake_invalidation_procedural_texture_count;
    uint64_t bake_invalidation_material_count;
    uint64_t bake_invalidation_shader_count;
    uint64_t bake_invalidation_animation_frame_count;
    uint64_t prebake_ready_procedural_texture_count;
    uint64_t prebake_ready_material_count;
} ResourceManagerStatsSnapshot;

void resource_manager_get_stats_snapshot(const ResourceManager* manager, ResourceManagerStatsSnapshot* snapshot);
size_t resource_manager_get_dirty_procedural_texture_count(const ResourceManager* manager);
size_t resource_manager_get_dirty_material_count(const ResourceManager* manager);
size_t resource_manager_get_dirty_shader_count(const ResourceManager* manager);
size_t resource_manager_get_dirty_baked_texture_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_cache_hit_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_cache_miss_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_procedural_texture_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_material_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_shader_count(const ResourceManager* manager);
uint64_t resource_manager_get_bake_invalidation_animation_frame_count(const ResourceManager* manager);
uint64_t resource_manager_get_prebake_ready_procedural_texture_count(const ResourceManager* manager);
uint64_t resource_manager_get_prebake_ready_material_count(const ResourceManager* manager);
size_t resource_manager_get_baked_texture_memory_bytes(const ResourceManager* manager);
size_t resource_manager_get_texture_count(const ResourceManager* manager);
size_t resource_manager_get_material_count(const ResourceManager* manager);
size_t resource_manager_get_shader_count(const ResourceManager* manager);
size_t resource_manager_get_procedural_texture_count(const ResourceManager* manager);
size_t resource_manager_get_baked_texture_count(const ResourceManager* manager);

#endif
