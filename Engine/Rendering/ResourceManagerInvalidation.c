#include "ResourceManagerInvalidationInternal.h"
#include "ResourceManagerStatsInternal.h"

#include <stdlib.h>

static bool resource_manager_reserve_handles(
    ResourceHandle** handles,
    size_t* capacity,
    size_t required
) {
    ResourceHandle* next_values;
    size_t next_capacity;

    if (*capacity >= required) {
        return true;
    }

    next_capacity = *capacity > 0U ? *capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (ResourceHandle*)realloc(*handles, next_capacity * sizeof(*next_values));
    if (next_values == NULL) {
        return false;
    }

    *handles = next_values;
    *capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_keys(
    BakedTextureKey** keys,
    size_t* capacity,
    size_t required
) {
    BakedTextureKey* next_values;
    size_t next_capacity;

    if (*capacity >= required) {
        return true;
    }

    next_capacity = *capacity > 0U ? *capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedTextureKey*)realloc(*keys, next_capacity * sizeof(*next_values));
    if (next_values == NULL) {
        return false;
    }

    *keys = next_values;
    *capacity = next_capacity;
    return true;
}

static bool resource_manager_append_unique_handle(
    ResourceHandle** handles,
    size_t* count,
    size_t* capacity,
    ResourceHandle handle
) {
    size_t index;

    if (handles == NULL || count == NULL || capacity == NULL || handle.id == 0U) {
        return false;
    }

    for (index = 0U; index < *count; ++index) {
        if ((*handles)[index].id == handle.id) {
            return true;
        }
    }

    if (!resource_manager_reserve_handles(handles, capacity, *count + 1U)) {
        return false;
    }

    (*handles)[(*count)++] = handle;
    return true;
}

static bool resource_manager_append_unique_baked_key(
    BakedTextureKey** keys,
    size_t* count,
    size_t* capacity,
    BakedTextureKey key
) {
    size_t index;

    if (keys == NULL || count == NULL || capacity == NULL) {
        return false;
    }

    for (index = 0U; index < *count; ++index) {
        if (resource_manager_baked_key_equals((*keys)[index], key)) {
            return true;
        }
    }

    if (!resource_manager_reserve_baked_keys(keys, capacity, *count + 1U)) {
        return false;
    }

    (*keys)[(*count)++] = key;
    return true;
}

void resource_manager_mark_dirty_procedural_texture(ResourceManager* manager, ResourceHandle handle) {
    if (manager == NULL || handle.id == 0U) {
        return;
    }

    if (resource_manager_append_unique_handle(
            &manager->invalidation->dirty_procedural_texture_handles,
            &manager->invalidation->dirty_procedural_texture_count,
            &manager->invalidation->dirty_procedural_texture_capacity,
            handle)) {
        manager->stats->bake_invalidation_procedural_texture_count += 1U;
    }
}

void resource_manager_mark_dirty_material(ResourceManager* manager, ResourceHandle handle) {
    if (manager == NULL || handle.id == 0U) {
        return;
    }

    if (resource_manager_append_unique_handle(
            &manager->invalidation->dirty_material_handles,
            &manager->invalidation->dirty_material_count,
            &manager->invalidation->dirty_material_capacity,
            handle)) {
        manager->stats->bake_invalidation_material_count += 1U;
    }
}

void resource_manager_mark_dirty_shader(ResourceManager* manager, ResourceHandle handle) {
    if (manager == NULL || handle.id == 0U) {
        return;
    }

    if (resource_manager_append_unique_handle(
            &manager->invalidation->dirty_shader_handles,
            &manager->invalidation->dirty_shader_count,
            &manager->invalidation->dirty_shader_capacity,
            handle)) {
        manager->stats->bake_invalidation_shader_count += 1U;
    }
}

void resource_manager_mark_dirty_baked_texture(ResourceManager* manager, BakedTextureKey key) {
    if (manager == NULL) {
        return;
    }

    (void)resource_manager_append_unique_baked_key(
        &manager->invalidation->dirty_baked_texture_keys,
        &manager->invalidation->dirty_baked_texture_count,
        &manager->invalidation->dirty_baked_texture_capacity,
        key
    );
}
