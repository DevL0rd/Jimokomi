#include "ResourceManagerBakeCacheInternal.h"
#include "ResourceManagerBakeQueueInternal.h"
#include "ResourceManagerInvalidationInternal.h"
#include "ResourceManagerRegistryInternal.h"
#include "ResourceManagerStatsInternal.h"

#include "../Core/Memory.h"

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
    manager->registry = (ResourceRegistryState*)calloc(1U, sizeof(*manager->registry));
    manager->bake_cache = (ResourceBakeCacheState*)calloc(1U, sizeof(*manager->bake_cache));
    manager->bake_queue = (ResourceBakeQueueState*)calloc(1U, sizeof(*manager->bake_queue));
    manager->invalidation = (ResourceInvalidationState*)calloc(1U, sizeof(*manager->invalidation));
    manager->stats = (ResourceStatsState*)calloc(1U, sizeof(*manager->stats));
    if (manager->registry == NULL ||
        manager->bake_cache == NULL ||
        manager->bake_queue == NULL ||
        manager->invalidation == NULL ||
        manager->stats == NULL) {
        resource_manager_dispose(manager);
        return false;
    }
    manager->backend = backend;
    manager->bake_queue->bake_time_budget_ms = 16.667;
    manager->bake_queue->bake_admission_total_hits = 1U;
    manager->bake_queue->bake_admission_frame_hits = 1U;
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

    if (manager->registry != NULL) {
        resource_manager_dispose_entries(manager->registry->textures, manager->registry->texture_count);
        resource_manager_dispose_entries(manager->registry->materials, manager->registry->material_count);
        resource_manager_dispose_entries(manager->registry->shaders, manager->registry->shader_count);
        resource_manager_dispose_entries(manager->registry->procedural_textures, manager->registry->procedural_texture_count);
        resource_manager_dispose_entries(manager->registry->procedural_meshes, manager->registry->procedural_mesh_count);
    }

    if (manager->backend != NULL && manager->backend->destroy_texture != NULL && manager->bake_cache != NULL) {
        size_t index;
        if (manager->registry != NULL) {
            for (index = 0U; index < manager->registry->texture_count; ++index) {
                manager->backend->destroy_texture(manager->backend->userdata, manager->registry->texture_values[index].texture);
            }
        }
        for (index = 0U; index < manager->bake_cache->baked_texture_count; ++index) {
            manager->backend->destroy_texture(manager->backend->userdata, manager->bake_cache->baked_textures[index].texture);
        }
    }

    if (manager->registry != NULL) {
        free(manager->registry->textures);
        free(manager->registry->texture_values);
        free(manager->registry->materials);
        free(manager->registry->material_values);
        free(manager->registry->shaders);
        free(manager->registry->shader_values);
        free(manager->registry->procedural_textures);
        free(manager->registry->procedural_texture_values);
        free(manager->registry->procedural_meshes);
        free(manager->registry->procedural_mesh_values);
    }
    if (manager->bake_cache != NULL) {
        size_t index;
        for (index = 0U; index < manager->bake_cache->baked_mesh_count; ++index) {
            Mesh* mesh = manager->bake_cache->baked_meshes[index].mesh;
            if (mesh != NULL) {
                free(mesh->triangles);
                free(mesh->lines);
                free(mesh);
            }
        }
        free(manager->bake_cache->baked_textures);
        free(manager->bake_cache->baked_texture_slots);
        free(manager->bake_cache->baked_meshes);
    }
    if (manager->bake_queue != NULL) {
        free(manager->bake_queue->static_pending_bake_requests);
        free(manager->bake_queue->transient_pending_bake_requests);
        free(manager->bake_queue->pending_bake_slots);
        free(manager->bake_queue->bake_interest_entries);
        free(manager->bake_queue->procedural_texture_last_requested_frame_indices);
    }
    if (manager->invalidation != NULL) {
        free(manager->invalidation->dirty_procedural_texture_handles);
        free(manager->invalidation->dirty_material_handles);
        free(manager->invalidation->dirty_shader_handles);
        free(manager->invalidation->dirty_baked_texture_keys);
    }
    free(manager->registry);
    free(manager->bake_cache);
    free(manager->bake_queue);
    free(manager->invalidation);
    free(manager->stats);
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
    manager->bake_queue->frame_serial += 1U;
    manager->bake_queue->bake_requests_this_frame = 0U;
    manager->invalidation->dirty_procedural_texture_count = 0U;
    manager->invalidation->dirty_material_count = 0U;
    manager->invalidation->dirty_shader_count = 0U;
    manager->invalidation->dirty_baked_texture_count = 0U;
    resource_manager_reset_empty_bake_queue(manager);
}

void resource_manager_set_bake_time_budget(ResourceManager* manager, double bake_time_budget_ms) {
    if (manager == NULL) {
        return;
    }

    manager->bake_queue->bake_time_budget_ms = bake_time_budget_ms > 0.0 ? bake_time_budget_ms : 0.0;
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
        manager->bake_queue->bake_admission_total_hits = total_hits;
    }
    if (frame_hits > 0U) {
        manager->bake_queue->bake_admission_frame_hits = frame_hits;
    }
}
