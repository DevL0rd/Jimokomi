#include "ResourceManagerInternal.h"

#include <stdlib.h>
#include <string.h>

static void resource_manager_dispose_entries(ResourceEntry* entries, size_t count) {
    size_t index;
    for (index = 0U; index < count; ++index) {
        free(entries[index].key);
    }
}

bool resource_manager_init(ResourceManager* manager, RenderBackend* backend) {
    if (manager == NULL) {
        return false;
    }
    memset(manager, 0, sizeof(*manager));
    manager->backend = backend;
    manager->bake_queue.bake_time_budget_ms = 16.667;
    manager->bake_queue.bake_admission_total_hits = 1U;
    manager->bake_queue.bake_admission_frame_hits = 1U;
    return true;
}

ResourceManager* resource_manager_create(RenderBackend* backend) {
    ResourceManager* manager = (ResourceManager*)calloc(1U, sizeof(*manager));

    if (manager == NULL) {
        return NULL;
    }
    if (!resource_manager_init(manager, backend)) {
        free(manager);
        return NULL;
    }
    return manager;
}

void resource_manager_dispose(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }

    resource_manager_dispose_entries(manager->registry.textures, manager->registry.texture_count);
    resource_manager_dispose_entries(manager->registry.materials, manager->registry.material_count);
    resource_manager_dispose_entries(manager->registry.shaders, manager->registry.shader_count);
    resource_manager_dispose_entries(manager->registry.visual_sources, manager->registry.visual_source_count);

    if (manager->backend != NULL && manager->backend->destroy_surface != NULL) {
        size_t index;
        for (index = 0U; index < manager->bake_cache.baked_surface_count; ++index) {
            manager->backend->destroy_surface(manager->backend->userdata, manager->bake_cache.baked_surfaces[index].surface);
        }
    }

    free(manager->registry.textures);
    free(manager->registry.texture_values);
    free(manager->registry.materials);
    free(manager->registry.material_values);
    free(manager->registry.shaders);
    free(manager->registry.shader_values);
    free(manager->registry.visual_sources);
    free(manager->registry.visual_source_values);
    free(manager->bake_cache.baked_surfaces);
    free(manager->bake_queue.static_pending_bake_requests);
    free(manager->bake_queue.transient_pending_bake_requests);
    free(manager->bake_cache.baked_surface_slots);
    free(manager->bake_queue.pending_bake_slots);
    free(manager->bake_queue.bake_interest_entries);
    free(manager->invalidation.dirty_visual_source_handles);
    free(manager->invalidation.dirty_material_handles);
    free(manager->invalidation.dirty_shader_handles);
    free(manager->invalidation.dirty_baked_surface_keys);
    free(manager->bake_queue.visual_source_last_requested_frame_indices);
    memset(manager, 0, sizeof(*manager));
}

void resource_manager_destroy(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }
    resource_manager_dispose(manager);
    free(manager);
}

void resource_manager_begin_frame(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }
    manager->bake_queue.frame_serial += 1U;
    manager->bake_queue.bake_requests_this_frame = 0U;
    manager->invalidation.dirty_visual_source_count = 0U;
    manager->invalidation.dirty_material_count = 0U;
    manager->invalidation.dirty_shader_count = 0U;
    manager->invalidation.dirty_baked_surface_count = 0U;
    resource_manager_reset_empty_bake_queue(manager);
}

void resource_manager_set_bake_time_budget(ResourceManager* manager, double bake_time_budget_ms) {
    if (manager == NULL) {
        return;
    }

    manager->bake_queue.bake_time_budget_ms = bake_time_budget_ms > 0.0 ? bake_time_budget_ms : 0.0;
}

void resource_manager_set_bake_admission_thresholds(
    ResourceManager* manager,
    size_t total_hits,
    size_t frame_hits
) {
    if (manager == NULL) {
        return;
    }

    if (total_hits > 0U) {
        manager->bake_queue.bake_admission_total_hits = total_hits;
    }
    if (frame_hits > 0U) {
        manager->bake_queue.bake_admission_frame_hits = frame_hits;
    }
}
