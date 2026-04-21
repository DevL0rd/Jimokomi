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
    manager->bake_time_budget_ms = 16.667;
    manager->bake_admission_total_hits = 1U;
    manager->bake_admission_frame_hits = 1U;
    return true;
}

void resource_manager_dispose(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }

    resource_manager_dispose_entries(manager->textures, manager->texture_count);
    resource_manager_dispose_entries(manager->materials, manager->material_count);
    resource_manager_dispose_entries(manager->shaders, manager->shader_count);
    resource_manager_dispose_entries(manager->visual_sources, manager->visual_source_count);

    if (manager->backend != NULL && manager->backend->destroy_surface != NULL) {
        size_t index;
        for (index = 0U; index < manager->baked_surface_count; ++index) {
            manager->backend->destroy_surface(manager->backend->userdata, manager->baked_surfaces[index].surface);
        }
    }

    free(manager->textures);
    free(manager->texture_values);
    free(manager->materials);
    free(manager->material_values);
    free(manager->shaders);
    free(manager->shader_values);
    free(manager->visual_sources);
    free(manager->visual_source_values);
    free(manager->baked_surfaces);
    free(manager->static_pending_bake_requests);
    free(manager->transient_pending_bake_requests);
    free(manager->baked_surface_slots);
    free(manager->pending_bake_slots);
    free(manager->bake_interest_entries);
    free(manager->dirty_visual_source_handles);
    free(manager->dirty_material_handles);
    free(manager->dirty_shader_handles);
    free(manager->dirty_baked_surface_keys);
    free(manager->visual_source_last_requested_frame_indices);
    memset(manager, 0, sizeof(*manager));
}

void resource_manager_begin_frame(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }
    manager->frame_serial += 1U;
    manager->bake_requests_this_frame = 0U;
    manager->dirty_visual_source_count = 0U;
    manager->dirty_material_count = 0U;
    manager->dirty_shader_count = 0U;
    manager->dirty_baked_surface_count = 0U;
    if (manager->static_pending_bake_request_head > 0U &&
        manager->static_pending_bake_request_head == manager->static_pending_bake_request_count) {
        manager->static_pending_bake_request_head = 0U;
        manager->static_pending_bake_request_count = 0U;
    }
    if (manager->transient_pending_bake_request_head > 0U &&
        manager->transient_pending_bake_request_head == manager->transient_pending_bake_request_count) {
        manager->transient_pending_bake_request_head = 0U;
        manager->transient_pending_bake_request_count = 0U;
    }
    if ((manager->static_pending_bake_request_count == 0U ||
         manager->static_pending_bake_request_head == manager->static_pending_bake_request_count) &&
        (manager->transient_pending_bake_request_count == 0U ||
         manager->transient_pending_bake_request_head == manager->transient_pending_bake_request_count)) {
        manager->pending_bake_slot_count = 0U;
        if (manager->pending_bake_slots != NULL && manager->pending_bake_slot_capacity > 0U) {
            memset(
                manager->pending_bake_slots,
                0,
                manager->pending_bake_slot_capacity * sizeof(*manager->pending_bake_slots)
            );
        }
    }
}

void resource_manager_set_bake_time_budget(ResourceManager* manager, double bake_time_budget_ms) {
    if (manager == NULL) {
        return;
    }

    manager->bake_time_budget_ms = bake_time_budget_ms > 0.0 ? bake_time_budget_ms : 0.0;
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
        manager->bake_admission_total_hits = total_hits;
    }
    if (frame_hits > 0U) {
        manager->bake_admission_frame_hits = frame_hits;
    }
}
