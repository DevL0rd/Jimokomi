#include "ResourceManagerInternal.h"

#include <string.h>

void resource_manager_get_stats_snapshot(const ResourceManager* manager, ResourceManagerStatsSnapshot* snapshot) {
    if (snapshot == NULL) {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    if (manager == NULL) {
        return;
    }

    snapshot->texture_count = manager->texture_count;
    snapshot->material_count = manager->material_count;
    snapshot->shader_count = manager->shader_count;
    snapshot->visual_source_count = manager->visual_source_count;
    snapshot->baked_surface_count = manager->baked_surface_count;
    snapshot->pending_bake_count = resource_manager_get_pending_bake_count(manager);
    snapshot->static_pending_bake_count = resource_manager_get_static_pending_bake_count(manager);
    snapshot->transient_pending_bake_count = resource_manager_get_transient_pending_bake_count(manager);
    snapshot->dirty_visual_source_count = manager->dirty_visual_source_count;
    snapshot->dirty_material_count = manager->dirty_material_count;
    snapshot->dirty_shader_count = manager->dirty_shader_count;
    snapshot->dirty_baked_surface_count = manager->dirty_baked_surface_count;
    snapshot->bake_interest_count = manager->bake_interest_count;
    snapshot->bake_budget_per_frame = manager->bake_budget_per_frame;
    snapshot->bake_requests_this_frame = manager->bake_requests_this_frame;
    snapshot->baked_surface_memory_bytes = resource_manager_get_baked_surface_memory_bytes(manager);
    snapshot->bake_cache_hits = manager->bake_cache_hits;
    snapshot->bake_cache_misses = manager->bake_cache_misses;
    snapshot->bake_invalidation_visual_source_count = manager->bake_invalidation_visual_source_count;
    snapshot->bake_invalidation_material_count = manager->bake_invalidation_material_count;
    snapshot->bake_invalidation_shader_count = manager->bake_invalidation_shader_count;
    snapshot->bake_invalidation_animation_frame_count = manager->bake_invalidation_animation_frame_count;
    snapshot->prebake_ready_visual_source_count = manager->prebake_ready_visual_source_count;
    snapshot->prebake_ready_material_count = manager->prebake_ready_material_count;
}

size_t resource_manager_get_pending_bake_count(const ResourceManager* manager) {
    return resource_manager_get_static_pending_bake_count(manager) +
           resource_manager_get_transient_pending_bake_count(manager);
}

bool resource_manager_has_pending_bakes(const ResourceManager* manager) {
    return resource_manager_get_pending_bake_count(manager) > 0U;
}

size_t resource_manager_get_static_pending_bake_count(const ResourceManager* manager) {
    if (manager == NULL || manager->static_pending_bake_request_count < manager->static_pending_bake_request_head) {
        return 0U;
    }
    return manager->static_pending_bake_request_count - manager->static_pending_bake_request_head;
}

size_t resource_manager_get_transient_pending_bake_count(const ResourceManager* manager) {
    if (manager == NULL || manager->transient_pending_bake_request_count < manager->transient_pending_bake_request_head) {
        return 0U;
    }
    return manager->transient_pending_bake_request_count - manager->transient_pending_bake_request_head;
}

size_t resource_manager_get_dirty_visual_source_count(const ResourceManager* manager) {
    return manager != NULL ? manager->dirty_visual_source_count : 0U;
}

size_t resource_manager_get_dirty_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->dirty_material_count : 0U;
}

size_t resource_manager_get_dirty_shader_count(const ResourceManager* manager) {
    return manager != NULL ? manager->dirty_shader_count : 0U;
}

size_t resource_manager_get_dirty_baked_surface_count(const ResourceManager* manager) {
    return manager != NULL ? manager->dirty_baked_surface_count : 0U;
}

uint64_t resource_manager_get_bake_cache_hit_count(const ResourceManager* manager) {
    return manager != NULL ? manager->bake_cache_hits : 0U;
}

uint64_t resource_manager_get_bake_cache_miss_count(const ResourceManager* manager) {
    return manager != NULL ? manager->bake_cache_misses : 0U;
}

uint64_t resource_manager_get_bake_invalidation_visual_source_count(const ResourceManager* manager) {
    return manager != NULL ? manager->bake_invalidation_visual_source_count : 0U;
}

uint64_t resource_manager_get_bake_invalidation_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->bake_invalidation_material_count : 0U;
}

uint64_t resource_manager_get_bake_invalidation_shader_count(const ResourceManager* manager) {
    return manager != NULL ? manager->bake_invalidation_shader_count : 0U;
}

uint64_t resource_manager_get_bake_invalidation_animation_frame_count(const ResourceManager* manager) {
    return manager != NULL ? manager->bake_invalidation_animation_frame_count : 0U;
}

uint64_t resource_manager_get_prebake_ready_visual_source_count(const ResourceManager* manager) {
    return manager != NULL ? manager->prebake_ready_visual_source_count : 0U;
}

uint64_t resource_manager_get_prebake_ready_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->prebake_ready_material_count : 0U;
}

size_t resource_manager_get_baked_surface_memory_bytes(const ResourceManager* manager) {
    size_t index;
    size_t total = 0U;

    if (manager == NULL) {
        return 0U;
    }

    for (index = 0U; index < manager->baked_surface_count; ++index) {
        const Surface* surface = manager->baked_surfaces[index].surface;
        if (surface == NULL) {
            continue;
        }
        total += (size_t)surface->width * (size_t)surface->height * surface->bytes_per_pixel;
    }

    return total;
}

size_t resource_manager_get_texture_count(const ResourceManager* manager) {
    return manager != NULL ? manager->texture_count : 0U;
}

size_t resource_manager_get_material_count(const ResourceManager* manager) {
    return manager != NULL ? manager->material_count : 0U;
}

size_t resource_manager_get_shader_count(const ResourceManager* manager) {
    return manager != NULL ? manager->shader_count : 0U;
}

size_t resource_manager_get_visual_source_count(const ResourceManager* manager) {
    return manager != NULL ? manager->visual_source_count : 0U;
}

size_t resource_manager_get_baked_surface_count(const ResourceManager* manager) {
    return manager != NULL ? manager->baked_surface_count : 0U;
}
