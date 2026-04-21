#include "ResourceManagerInternal.h"

#include <stdlib.h>

bool resource_manager_reserve_baked_surfaces(
    ResourceManager* manager,
    size_t required
) {
    BakedSurfaceResource* next_values;
    size_t next_capacity;

    if (manager->bake_cache.baked_surface_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_cache.baked_surface_capacity > 0U ? manager->bake_cache.baked_surface_capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedSurfaceResource*)realloc(
        manager->bake_cache.baked_surfaces,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->bake_cache.baked_surfaces = next_values;
    manager->bake_cache.baked_surface_capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_surface_slots(
    ResourceManager* manager,
    size_t required
) {
    BakedSurfaceSlot* next_slots;
    size_t next_capacity;
    size_t index;

    if (manager->bake_cache.baked_surface_slot_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_cache.baked_surface_slot_capacity > 0U ? manager->bake_cache.baked_surface_slot_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_slots = (BakedSurfaceSlot*)calloc(next_capacity, sizeof(*next_slots));
    if (next_slots == NULL) {
        return false;
    }

    for (index = 0U; index < manager->bake_cache.baked_surface_slot_capacity; ++index) {
        BakedSurfaceSlot* slot = &manager->bake_cache.baked_surface_slots[index];
        if (slot->state != SLOT_STATE_FILLED) {
            continue;
        }

        {
            size_t slot_index = (size_t)(resource_manager_hash_baked_key(slot->key) & (uint64_t)(next_capacity - 1U));
            while (next_slots[slot_index].state == SLOT_STATE_FILLED) {
                slot_index = (slot_index + 1U) & (next_capacity - 1U);
            }
            next_slots[slot_index] = *slot;
        }
    }

    free(manager->bake_cache.baked_surface_slots);
    manager->bake_cache.baked_surface_slots = next_slots;
    manager->bake_cache.baked_surface_slot_capacity = next_capacity;
    return true;
}

bool resource_manager_insert_baked_surface_slot(
    ResourceManager* manager,
    BakedSurfaceKey key,
    size_t value_index
) {
    size_t slot_index;

    if (manager == NULL) {
        return false;
    }

    if ((manager->bake_cache.baked_surface_slot_count + 1U) * 10U >= manager->bake_cache.baked_surface_slot_capacity * 7U) {
        if (!resource_manager_reserve_baked_surface_slots(
                manager,
                manager->bake_cache.baked_surface_slot_capacity > 0U
                    ? manager->bake_cache.baked_surface_slot_capacity * 2U
                    : 64U)) {
            return false;
        }
    }

    if (manager->bake_cache.baked_surface_slot_capacity == 0U &&
        !resource_manager_reserve_baked_surface_slots(manager, 64U)) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_cache.baked_surface_slot_capacity - 1U));
    while (manager->bake_cache.baked_surface_slots[slot_index].state == SLOT_STATE_FILLED) {
        slot_index = (slot_index + 1U) & (manager->bake_cache.baked_surface_slot_capacity - 1U);
    }

    manager->bake_cache.baked_surface_slots[slot_index].state = SLOT_STATE_FILLED;
    manager->bake_cache.baked_surface_slots[slot_index].key = key;
    manager->bake_cache.baked_surface_slots[slot_index].index = value_index;
    manager->bake_cache.baked_surface_slot_count += 1U;
    return true;
}

const Surface* resource_manager_find_baked_surface(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->bake_cache.baked_surface_slot_capacity == 0U) {
        return NULL;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_cache.baked_surface_slot_capacity - 1U));
    while (manager->bake_cache.baked_surface_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->bake_cache.baked_surface_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->bake_cache.baked_surface_slots[slot_index].key, key)) {
            return manager->bake_cache.baked_surfaces[manager->bake_cache.baked_surface_slots[slot_index].index].surface;
        }
        slot_index = (slot_index + 1U) & (manager->bake_cache.baked_surface_slot_capacity - 1U);
    }

    return NULL;
}

bool resource_manager_store_baked_surface(
    ResourceManager* manager,
    BakedSurfaceKey key,
    Surface* surface
) {
    size_t index;

    if (manager == NULL || surface == NULL) {
        return false;
    }

    if (!resource_manager_reserve_baked_surfaces(manager, manager->bake_cache.baked_surface_count + 1U)) {
        return false;
    }

    index = manager->bake_cache.baked_surface_count++;
    manager->bake_cache.baked_surfaces[index].key = key;
    manager->bake_cache.baked_surfaces[index].surface = surface;
    if (!resource_manager_insert_baked_surface_slot(manager, key, index)) {
        manager->bake_cache.baked_surface_count -= 1U;
        manager->bake_cache.baked_surfaces[index].surface = NULL;
        return false;
    }

    resource_manager_mark_dirty_baked_surface(manager, key);
    return true;
}

const Surface* resource_manager_get_baked_surface(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedSurfacePass pass,
    void* user_data
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    const Surface* baked_surface;
    BakedSurfaceKey key;

    if (manager == NULL || manager->backend == NULL ||
        manager->backend->create_surface == NULL ||
        manager->backend->set_target == NULL ||
        manager->backend->reset_target == NULL ||
        manager->backend->clear == NULL) {
        return NULL;
    }

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    material = resource_manager_get_material(manager, material_handle);
    if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return NULL;
    }

    key.visual_source_id = visual_source_handle.id;
    key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
    key.shader_id = shader_handle.id;
    key.frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.pass = (uint8_t)pass;

    baked_surface = resource_manager_find_baked_surface(manager, key);
    if (baked_surface != NULL) {
        manager->stats.bake_cache_hits += 1U;
        return baked_surface;
    }

    manager->stats.bake_cache_misses += 1U;
    (void)user_data;
    return NULL;
}
