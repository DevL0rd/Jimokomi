#include "ResourceManagerBakeCacheInternal.h"
#include "ResourceManagerBakeQueueInternal.h"
#include "ResourceManagerInvalidationInternal.h"
#include "ResourceManagerRegistryInternal.h"
#include "ResourceManagerStatsInternal.h"

#include <string.h>

void resource_manager_get_stats_snapshot(const ResourceManager* manager, ResourceManagerStatsSnapshot* snapshot) {
    if (snapshot == NULL) {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    if (manager == NULL) {
        return;
    }

    snapshot->texture_count = manager->registry->texture_count;
    snapshot->material_count = manager->registry->material_count;
    snapshot->shader_count = manager->registry->shader_count;
    snapshot->procedural_texture_count = manager->registry->procedural_texture_count;
    snapshot->baked_texture_count = manager->bake_cache->baked_texture_count;
    snapshot->pending_bake_count = resource_manager_get_pending_bake_count(manager);
    snapshot->static_pending_bake_count = resource_manager_get_static_pending_bake_count(manager);
    snapshot->transient_pending_bake_count = resource_manager_get_transient_pending_bake_count(manager);
    snapshot->dirty_procedural_texture_count = manager->invalidation->dirty_procedural_texture_count;
    snapshot->dirty_material_count = manager->invalidation->dirty_material_count;
    snapshot->dirty_shader_count = manager->invalidation->dirty_shader_count;
    snapshot->dirty_baked_texture_count = manager->invalidation->dirty_baked_texture_count;
    snapshot->bake_interest_count = manager->bake_queue->bake_interest_count;
    snapshot->bake_requests_this_frame = manager->bake_queue->bake_requests_this_frame;
    snapshot->bake_time_budget_ms = manager->bake_queue->bake_time_budget_ms;
    snapshot->bake_process_ms = manager->bake_queue->last_bake_process_ms;
    snapshot->baked_texture_memory_bytes = resource_manager_get_baked_texture_memory_bytes(manager);
    snapshot->bake_cache_hits = manager->stats->bake_cache_hits;
    snapshot->bake_cache_misses = manager->stats->bake_cache_misses;
    snapshot->bake_invalidation_procedural_texture_count = manager->stats->bake_invalidation_procedural_texture_count;
    snapshot->bake_invalidation_material_count = manager->stats->bake_invalidation_material_count;
    snapshot->bake_invalidation_shader_count = manager->stats->bake_invalidation_shader_count;
    snapshot->bake_invalidation_animation_frame_count = manager->stats->bake_invalidation_animation_frame_count;
    snapshot->prebake_ready_procedural_texture_count = manager->stats->prebake_ready_procedural_texture_count;
    snapshot->prebake_ready_material_count = manager->stats->prebake_ready_material_count;
}

size_t resource_manager_get_pending_bake_count(const ResourceManager* manager) {
    return resource_manager_get_static_pending_bake_count(manager) +
           resource_manager_get_transient_pending_bake_count(manager);
}

bool resource_manager_has_pending_bakes(const ResourceManager* manager) {
    return resource_manager_get_pending_bake_count(manager) > 0U;
}

size_t resource_manager_get_static_pending_bake_count(const ResourceManager* manager) {
    if (manager == NULL || manager->bake_queue->static_pending_bake_request_count < manager->bake_queue->static_pending_bake_request_head) {
        return 0U;
    }
    return manager->bake_queue->static_pending_bake_request_count - manager->bake_queue->static_pending_bake_request_head;
}

size_t resource_manager_get_transient_pending_bake_count(const ResourceManager* manager) {
    if (manager == NULL || manager->bake_queue->transient_pending_bake_request_count < manager->bake_queue->transient_pending_bake_request_head) {
        return 0U;
    }
    return manager->bake_queue->transient_pending_bake_request_count - manager->bake_queue->transient_pending_bake_request_head;
}

size_t resource_manager_get_dirty_procedural_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->invalidation->dirty_procedural_texture_count : 0U;
}

size_t resource_manager_get_dirty_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->invalidation->dirty_material_count : 0U;
}

size_t resource_manager_get_dirty_shader_count(const ResourceManager* manager) {
    return manager != NULL ? manager->invalidation->dirty_shader_count : 0U;
}

size_t resource_manager_get_dirty_baked_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->invalidation->dirty_baked_texture_count : 0U;
}

uint64_t resource_manager_get_bake_cache_hit_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->bake_cache_hits : 0U;
}

uint64_t resource_manager_get_bake_cache_miss_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->bake_cache_misses : 0U;
}

uint64_t resource_manager_get_bake_invalidation_procedural_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->bake_invalidation_procedural_texture_count : 0U;
}

uint64_t resource_manager_get_bake_invalidation_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->bake_invalidation_material_count : 0U;
}

uint64_t resource_manager_get_bake_invalidation_shader_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->bake_invalidation_shader_count : 0U;
}

uint64_t resource_manager_get_bake_invalidation_animation_frame_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->bake_invalidation_animation_frame_count : 0U;
}

uint64_t resource_manager_get_prebake_ready_procedural_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->prebake_ready_procedural_texture_count : 0U;
}

uint64_t resource_manager_get_prebake_ready_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->stats->prebake_ready_material_count : 0U;
}

size_t resource_manager_get_baked_texture_memory_bytes(const ResourceManager* manager) {
    size_t index;
    size_t total = 0U;

    if (manager == NULL) {
        return 0U;
    }

    for (index = 0U; index < manager->bake_cache->baked_texture_count; ++index) {
        const Texture* texture = manager->bake_cache->baked_textures[index].texture;
        if (texture == NULL) {
            continue;
        }
        total += (size_t)texture->width * (size_t)texture->height * texture->bytes_per_pixel;
    }

    return total;
}

size_t resource_manager_get_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->registry->texture_count : 0U;
}

size_t resource_manager_get_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->registry->material_count : 0U;
}

size_t resource_manager_get_shader_count(const ResourceManager* manager) {
    return manager != NULL ? manager->registry->shader_count : 0U;
}

size_t resource_manager_get_procedural_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->registry->procedural_texture_count : 0U;
}

size_t resource_manager_get_baked_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->bake_cache->baked_texture_count : 0U;
}
