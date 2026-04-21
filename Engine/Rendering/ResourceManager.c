#include "ResourceManagerInternal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {
    SLOT_STATE_EMPTY = 0,
    SLOT_STATE_FILLED = 1,
    SLOT_STATE_TOMBSTONE = 2
};

static bool resource_manager_baked_key_equals(BakedSurfaceKey a, BakedSurfaceKey b);

static bool resource_manager_reserve(
    ResourceEntry** entries,
    void** values,
    size_t element_size,
    size_t* capacity,
    size_t required
) {
    ResourceEntry* next_entries;
    void* next_values;
    size_t next_capacity;

    if (*capacity >= required) {
        return true;
    }

    next_capacity = *capacity > 0U ? *capacity : 8U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_entries = (ResourceEntry*)realloc(*entries, next_capacity * sizeof(*next_entries));
    if (next_entries == NULL) {
        return false;
    }

    next_values = realloc(*values, next_capacity * element_size);
    if (next_values == NULL) {
        *entries = next_entries;
        return false;
    }

    *entries = next_entries;
    *values = next_values;
    *capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_surfaces(
    ResourceManager* manager,
    size_t required
) {
    BakedSurfaceResource* next_values;
    size_t next_capacity;

    if (manager->baked_surface_capacity >= required) {
        return true;
    }

    next_capacity = manager->baked_surface_capacity > 0U ? manager->baked_surface_capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedSurfaceResource*)realloc(
        manager->baked_surfaces,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->baked_surfaces = next_values;
    manager->baked_surface_capacity = next_capacity;
    return true;
}

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
    BakedSurfaceKey** keys,
    size_t* capacity,
    size_t required
) {
    BakedSurfaceKey* next_values;
    size_t next_capacity;

    if (*capacity >= required) {
        return true;
    }

    next_capacity = *capacity > 0U ? *capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedSurfaceKey*)realloc(*keys, next_capacity * sizeof(*next_values));
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
    BakedSurfaceKey** keys,
    size_t* count,
    size_t* capacity,
    BakedSurfaceKey key
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

static void resource_manager_mark_dirty_visual_source(ResourceManager* manager, ResourceHandle handle) {
    if (manager == NULL || handle.id == 0U) {
        return;
    }

    if (resource_manager_append_unique_handle(
            &manager->dirty_visual_source_handles,
            &manager->dirty_visual_source_count,
            &manager->dirty_visual_source_capacity,
            handle)) {
        manager->bake_invalidation_visual_source_count += 1U;
    }
}

static void resource_manager_mark_dirty_material(ResourceManager* manager, ResourceHandle handle) {
    if (manager == NULL || handle.id == 0U) {
        return;
    }

    if (resource_manager_append_unique_handle(
            &manager->dirty_material_handles,
            &manager->dirty_material_count,
            &manager->dirty_material_capacity,
            handle)) {
        manager->bake_invalidation_material_count += 1U;
    }
}

static void resource_manager_mark_dirty_shader(ResourceManager* manager, ResourceHandle handle) {
    if (manager == NULL || handle.id == 0U) {
        return;
    }

    if (resource_manager_append_unique_handle(
            &manager->dirty_shader_handles,
            &manager->dirty_shader_count,
            &manager->dirty_shader_capacity,
            handle)) {
        manager->bake_invalidation_shader_count += 1U;
    }
}

static void resource_manager_mark_dirty_baked_surface(ResourceManager* manager, BakedSurfaceKey key) {
    if (manager == NULL) {
        return;
    }

    (void)resource_manager_append_unique_baked_key(
        &manager->dirty_baked_surface_keys,
        &manager->dirty_baked_surface_count,
        &manager->dirty_baked_surface_capacity,
        key
    );
}

static uint64_t resource_manager_hash_baked_key(BakedSurfaceKey key) {
    uint64_t hash = 1469598103934665603ULL;
    hash ^= (uint64_t)key.visual_source_id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.material_id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.shader_id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.frame_index;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.pass;
    hash *= 1099511628211ULL;
    return hash;
}

static bool resource_manager_baked_key_equals(BakedSurfaceKey a, BakedSurfaceKey b) {
    return a.visual_source_id == b.visual_source_id &&
           a.material_id == b.material_id &&
           a.shader_id == b.shader_id &&
           a.frame_index == b.frame_index &&
           a.pass == b.pass;
}

static bool resource_manager_reserve_baked_surface_slots(
    ResourceManager* manager,
    size_t required
) {
    BakedSurfaceSlot* next_slots;
    size_t next_capacity;
    size_t index;

    if (manager->baked_surface_slot_capacity >= required) {
        return true;
    }

    next_capacity = manager->baked_surface_slot_capacity > 0U ? manager->baked_surface_slot_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_slots = (BakedSurfaceSlot*)calloc(next_capacity, sizeof(*next_slots));
    if (next_slots == NULL) {
        return false;
    }

    for (index = 0U; index < manager->baked_surface_slot_capacity; ++index) {
        BakedSurfaceSlot* slot = &manager->baked_surface_slots[index];
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

    free(manager->baked_surface_slots);
    manager->baked_surface_slots = next_slots;
    manager->baked_surface_slot_capacity = next_capacity;
    return true;
}

static bool resource_manager_insert_baked_surface_slot(
    ResourceManager* manager,
    BakedSurfaceKey key,
    size_t value_index
) {
    size_t slot_index;

    if (manager == NULL) {
        return false;
    }

    if ((manager->baked_surface_slot_count + 1U) * 10U >= manager->baked_surface_slot_capacity * 7U) {
        if (!resource_manager_reserve_baked_surface_slots(
                manager,
                manager->baked_surface_slot_capacity > 0U
                    ? manager->baked_surface_slot_capacity * 2U
                    : 64U)) {
            return false;
        }
    }

    if (manager->baked_surface_slot_capacity == 0U &&
        !resource_manager_reserve_baked_surface_slots(manager, 64U)) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->baked_surface_slot_capacity - 1U));
    while (manager->baked_surface_slots[slot_index].state == SLOT_STATE_FILLED) {
        slot_index = (slot_index + 1U) & (manager->baked_surface_slot_capacity - 1U);
    }

    manager->baked_surface_slots[slot_index].state = SLOT_STATE_FILLED;
    manager->baked_surface_slots[slot_index].key = key;
    manager->baked_surface_slots[slot_index].index = value_index;
    manager->baked_surface_slot_count += 1U;
    return true;
}

static bool resource_manager_reserve_pending_queue(
    PendingBakeRequest** requests,
    size_t* capacity,
    size_t required
) {
    PendingBakeRequest* next_values;
    size_t next_capacity;

    if (*capacity >= required) {
        return true;
    }

    next_capacity = *capacity > 0U ? *capacity : 32U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (PendingBakeRequest*)realloc(
        *requests,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    *requests = next_values;
    *capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_pending_bake_slots(
    ResourceManager* manager,
    size_t required
) {
    PendingBakeSlot* next_slots;
    size_t next_capacity;
    size_t index;

    if (manager->pending_bake_slot_capacity >= required) {
        return true;
    }

    next_capacity = manager->pending_bake_slot_capacity > 0U ? manager->pending_bake_slot_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_slots = (PendingBakeSlot*)calloc(next_capacity, sizeof(*next_slots));
    if (next_slots == NULL) {
        return false;
    }

    for (index = 0U; index < manager->pending_bake_slot_capacity; ++index) {
        PendingBakeSlot* slot = &manager->pending_bake_slots[index];
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

    free(manager->pending_bake_slots);
    manager->pending_bake_slots = next_slots;
    manager->pending_bake_slot_capacity = next_capacity;
    return true;
}

static bool resource_manager_pending_bake_contains(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->pending_bake_slot_capacity == 0U) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->pending_bake_slot_capacity - 1U));
    while (manager->pending_bake_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->pending_bake_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->pending_bake_slots[slot_index].key, key)) {
            return true;
        }
        slot_index = (slot_index + 1U) & (manager->pending_bake_slot_capacity - 1U);
    }

    return false;
}

static bool resource_manager_insert_pending_bake_slot(
    ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t slot_index;
    size_t first_tombstone = (size_t)-1;

    if (manager == NULL) {
        return false;
    }

    if ((manager->pending_bake_slot_count + 1U) * 10U >= manager->pending_bake_slot_capacity * 7U) {
        if (!resource_manager_reserve_pending_bake_slots(
                manager,
                manager->pending_bake_slot_capacity > 0U
                    ? manager->pending_bake_slot_capacity * 2U
                    : 64U)) {
            return false;
        }
    }

    if (manager->pending_bake_slot_capacity == 0U &&
        !resource_manager_reserve_pending_bake_slots(manager, 64U)) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->pending_bake_slot_capacity - 1U));
    while (manager->pending_bake_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->pending_bake_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->pending_bake_slots[slot_index].key, key)) {
            return true;
        }
        if (first_tombstone == (size_t)-1 &&
            manager->pending_bake_slots[slot_index].state == SLOT_STATE_TOMBSTONE) {
            first_tombstone = slot_index;
        }
        slot_index = (slot_index + 1U) & (manager->pending_bake_slot_capacity - 1U);
    }

    if (first_tombstone != (size_t)-1) {
        slot_index = first_tombstone;
    }

    manager->pending_bake_slots[slot_index].state = SLOT_STATE_FILLED;
    manager->pending_bake_slots[slot_index].key = key;
    manager->pending_bake_slot_count += 1U;
    return true;
}

static void resource_manager_remove_pending_bake_slot(
    ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->pending_bake_slot_capacity == 0U) {
        return;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->pending_bake_slot_capacity - 1U));
    while (manager->pending_bake_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->pending_bake_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->pending_bake_slots[slot_index].key, key)) {
            manager->pending_bake_slots[slot_index].state = SLOT_STATE_TOMBSTONE;
            manager->pending_bake_slot_count -= 1U;
            return;
        }
        slot_index = (slot_index + 1U) & (manager->pending_bake_slot_capacity - 1U);
    }
}

static bool resource_manager_reserve_bake_interest_entries(
    ResourceManager* manager,
    size_t required
) {
    BakeInterestEntry* next_values;
    size_t next_capacity;

    if (manager->bake_interest_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_interest_capacity > 0U ? manager->bake_interest_capacity : 32U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakeInterestEntry*)realloc(
        manager->bake_interest_entries,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->bake_interest_entries = next_values;
    manager->bake_interest_capacity = next_capacity;
    return true;
}

static ResourceHandle resource_handle(uint32_t id) {
    ResourceHandle handle;
    handle.id = id;
    return handle;
}

static ResourceHandle resource_manager_find(
    const ResourceEntry* entries,
    size_t count,
    const char* key
) {
    size_t index;

    if (key == NULL) {
        return resource_handle(0U);
    }

    for (index = 0U; index < count; ++index) {
        if (entries[index].key != NULL && strcmp(entries[index].key, key) == 0) {
            return resource_handle(entries[index].id);
        }
    }

    return resource_handle(0U);
}

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
    manager->bake_budget_per_frame = 500U;
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

void resource_manager_set_bake_budget(ResourceManager* manager, size_t bake_budget_per_frame) {
    if (manager == NULL) {
        return;
    }

    manager->bake_budget_per_frame = bake_budget_per_frame;
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

ResourceHandle resource_manager_register_texture(
    ResourceManager* manager,
    const char* key,
    Surface* surface
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || surface == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->textures, manager->texture_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->textures,
        (void**)&manager->texture_values,
        sizeof(*manager->texture_values),
        &manager->texture_capacity,
        manager->texture_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->texture_count++;
    manager->textures[index].key = string_duplicate(key);
    manager->textures[index].id = (uint32_t)(index + 1U);
    manager->texture_values[index].surface = surface;
    return resource_handle(manager->textures[index].id);
}

ResourceHandle resource_manager_register_material(
    ResourceManager* manager,
    const char* key,
    const Material* material
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || material == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->materials, manager->material_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->materials,
        (void**)&manager->material_values,
        sizeof(*manager->material_values),
        &manager->material_capacity,
        manager->material_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->material_count++;
    manager->materials[index].key = string_duplicate(key);
    manager->materials[index].id = (uint32_t)(index + 1U);
    manager->material_values[index].value = *material;
    resource_manager_mark_dirty_material(manager, resource_handle(manager->materials[index].id));
    return resource_handle(manager->materials[index].id);
}

ResourceHandle resource_manager_register_shader(
    ResourceManager* manager,
    const char* key,
    ShaderStyle style
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->shaders, manager->shader_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->shaders,
        (void**)&manager->shader_values,
        sizeof(*manager->shader_values),
        &manager->shader_capacity,
        manager->shader_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->shader_count++;
    manager->shaders[index].key = string_duplicate(key);
    manager->shaders[index].id = (uint32_t)(index + 1U);
    manager->shader_values[index].style = style;
    resource_manager_mark_dirty_shader(manager, resource_handle(manager->shaders[index].id));
    return resource_handle(manager->shaders[index].id);
}

ResourceHandle resource_manager_register_procedural_source(
    ResourceManager* manager,
    const char* key,
    const ProceduralSourceDesc* desc
) {
    ResourceHandle existing;
    size_t index;

    if (manager == NULL || key == NULL || desc == NULL) {
        return resource_handle(0U);
    }

    existing = resource_manager_find(manager->visual_sources, manager->visual_source_count, key);
    if (existing.id != 0U) {
        return existing;
    }

    if (!resource_manager_reserve(
        &manager->visual_sources,
        (void**)&manager->visual_source_values,
        sizeof(*manager->visual_source_values),
        &manager->visual_source_capacity,
        manager->visual_source_count + 1U
    )) {
        return resource_handle(0U);
    }

    index = manager->visual_source_count++;
    manager->visual_sources[index].key = string_duplicate(key);
    manager->visual_sources[index].id = (uint32_t)(index + 1U);
    manager->visual_source_values[index].kind = VISUAL_KIND_PROCEDURAL_TEXTURE;
    manager->visual_source_values[index].width = desc->width;
    manager->visual_source_values[index].height = desc->height;
    manager->visual_source_values[index].animation_fps = desc->animation_fps;
    manager->visual_source_values[index].bake_animation_fps = desc->bake_animation_fps;
    manager->visual_source_values[index].loop = desc->loop;
    manager->visual_source_values[index].bake_policy = desc->bake_policy;
    manager->visual_source_values[index].prebake_required = desc->prebake_required;
    manager->visual_source_values[index].bake_instance_invariant = desc->bake_instance_invariant;
    manager->visual_source_values[index].bake_ignores_material = desc->bake_ignores_material;
    manager->visual_source_values[index].bake_frame_count = desc->bake_frame_count;
    manager->visual_source_values[index].draw_body = desc->draw_body;
    manager->visual_source_values[index].draw_overlay = desc->draw_overlay;
    manager->visual_source_values[index].texture_handle = resource_handle(0U);
    if (manager->visual_source_last_requested_frame_capacity < manager->visual_source_capacity) {
        uint32_t* next_frames = (uint32_t*)realloc(
            manager->visual_source_last_requested_frame_indices,
            manager->visual_source_capacity * sizeof(*next_frames)
        );
        if (next_frames != NULL) {
            size_t fill_index;
            for (fill_index = manager->visual_source_last_requested_frame_capacity;
                 fill_index < manager->visual_source_capacity;
                 ++fill_index) {
                next_frames[fill_index] = UINT32_MAX;
            }
            manager->visual_source_last_requested_frame_indices = next_frames;
            manager->visual_source_last_requested_frame_capacity = manager->visual_source_capacity;
        }
    }
    resource_manager_mark_dirty_visual_source(manager, resource_handle(manager->visual_sources[index].id));
    if (desc->prebake_required) {
        manager->prebake_ready_visual_source_count += 1U;
    }
    return resource_handle(manager->visual_sources[index].id);
}

const TextureResource* resource_manager_get_texture(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->texture_count) {
        return NULL;
    }
    return &manager->texture_values[handle.id - 1U];
}

const MaterialResource* resource_manager_get_material(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->material_count) {
        return NULL;
    }
    return &manager->material_values[handle.id - 1U];
}

const ShaderResource* resource_manager_get_shader(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->shader_count) {
        return NULL;
    }
    return &manager->shader_values[handle.id - 1U];
}

const VisualSourceResource* resource_manager_get_visual_source(
    const ResourceManager* manager,
    ResourceHandle handle
) {
    if (manager == NULL || handle.id == 0U || handle.id > manager->visual_source_count) {
        return NULL;
    }
    return &manager->visual_source_values[handle.id - 1U];
}

static uint32_t resource_manager_normalize_frame_index(
    const VisualSourceResource* source,
    uint32_t frame_index
) {
    uint32_t baked_frame_index = frame_index;
    uint32_t baked_frame_count;

    if (source == NULL) {
        return frame_index;
    }

    if (source->animation_fps > 0.0f && source->bake_animation_fps > 0.0f) {
        float time_seconds = (float)frame_index / source->animation_fps;
        baked_frame_index = (uint32_t)floorf(time_seconds * source->bake_animation_fps);
    }

    baked_frame_count = 0U;
    if (source->bake_frame_count > 0U) {
        if (source->animation_fps > 0.0f && source->bake_animation_fps > 0.0f) {
            float loop_duration_seconds = (float)source->bake_frame_count / source->animation_fps;
            baked_frame_count = (uint32_t)ceilf(loop_duration_seconds * source->bake_animation_fps);
        } else {
            baked_frame_count = source->bake_frame_count;
        }
    }

    if (source->loop && baked_frame_count > 0U) {
        return baked_frame_index % baked_frame_count;
    }
    return baked_frame_index;
}

static bool resource_manager_is_bake_eligible(
    const VisualSourceResource* source,
    BakedSurfacePass pass
) {
    if (source == NULL) {
        return false;
    }
    if (source->bake_policy == BAKE_POLICY_DISABLED || !source->bake_instance_invariant) {
        return false;
    }
    if (pass == BAKED_SURFACE_PASS_BODY) {
        return source->draw_body != NULL;
    }
    if (pass == BAKED_SURFACE_PASS_OVERLAY) {
        return source->draw_overlay != NULL;
    }
    return false;
}

static size_t resource_manager_get_bake_interest_priority(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t index;

    if (manager == NULL) {
        return 0U;
    }

    for (index = 0U; index < manager->bake_interest_count; ++index) {
        if (resource_manager_baked_key_equals(manager->bake_interest_entries[index].key, key)) {
            return (size_t)manager->bake_interest_entries[index].frame_hits * 4U +
                   (size_t)manager->bake_interest_entries[index].total_hits;
        }
    }

    return 0U;
}

static bool resource_manager_enqueue_pending_request(
    PendingBakeRequest** requests,
    size_t* count,
    size_t* capacity,
    BakedSurfaceKey key,
    uint32_t priority
) {
    size_t insert_index;

    if (requests == NULL || count == NULL || capacity == NULL) {
        return false;
    }

    if (!resource_manager_reserve_pending_queue(requests, capacity, *count + 1U)) {
        return false;
    }

    insert_index = *count;
    while (insert_index > 0U && (*requests)[insert_index - 1U].priority < priority) {
        (*requests)[insert_index] = (*requests)[insert_index - 1U];
        insert_index -= 1U;
    }
    (*requests)[insert_index].key = key;
    (*requests)[insert_index].priority = priority;
    *count += 1U;
    return true;
}

static const Surface* resource_manager_find_baked_surface(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->baked_surface_slot_capacity == 0U) {
        return NULL;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->baked_surface_slot_capacity - 1U));
    while (manager->baked_surface_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->baked_surface_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->baked_surface_slots[slot_index].key, key)) {
            return manager->baked_surfaces[manager->baked_surface_slots[slot_index].index].surface;
        }
        slot_index = (slot_index + 1U) & (manager->baked_surface_slot_capacity - 1U);
    }

    return NULL;
}

static bool resource_manager_has_pending_bake(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    return resource_manager_pending_bake_contains(manager, key);
}

static bool resource_manager_should_admit_bake(
    ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t index;
    BakeInterestEntry* entry;

    if (manager == NULL) {
        return false;
    }

    for (index = 0U; index < manager->bake_interest_count; ++index) {
        if (resource_manager_baked_key_equals(manager->bake_interest_entries[index].key, key)) {
            entry = &manager->bake_interest_entries[index];
            if (entry->last_seen_frame != manager->frame_serial) {
                entry->frame_hits = 0U;
                entry->last_seen_frame = manager->frame_serial;
            }
            entry->frame_hits += 1U;
            entry->total_hits += 1U;
            return entry->frame_hits >= manager->bake_admission_frame_hits ||
                   entry->total_hits >= manager->bake_admission_total_hits;
        }
    }

    if (!resource_manager_reserve_bake_interest_entries(manager, manager->bake_interest_count + 1U)) {
        return false;
    }

    entry = &manager->bake_interest_entries[manager->bake_interest_count++];
    memset(entry, 0, sizeof(*entry));
    entry->key = key;
    entry->total_hits = 1U;
    entry->frame_hits = 1U;
    entry->last_seen_frame = manager->frame_serial;
    return entry->frame_hits >= manager->bake_admission_frame_hits ||
           entry->total_hits >= manager->bake_admission_total_hits;
}

static bool resource_manager_enqueue_baked_request(
    ResourceManager* manager,
    BakedSurfaceKey key,
    bool bypass_admission
) {
    const VisualSourceResource* source;
    uint32_t priority;

    if (manager == NULL) {
        return false;
    }
    if (resource_manager_find_baked_surface(manager, key) != NULL ||
        resource_manager_has_pending_bake(manager, key)) {
        return false;
    }
    if (!bypass_admission && !resource_manager_should_admit_bake(manager, key)) {
        return false;
    }
    if (!resource_manager_insert_pending_bake_slot(manager, key)) {
        return false;
    }

    source = resource_manager_get_visual_source(manager, resource_handle(key.visual_source_id));
    priority = (uint32_t)resource_manager_get_bake_interest_priority(manager, key);
    if (source != NULL && source->prebake_required) {
        priority += 1000000U;
        if (!resource_manager_enqueue_pending_request(
            &manager->static_pending_bake_requests,
            &manager->static_pending_bake_request_count,
            &manager->static_pending_bake_request_capacity,
            key,
            priority
        )) {
            resource_manager_remove_pending_bake_slot(manager, key);
            return false;
        }
        return true;
    }

    if (!resource_manager_enqueue_pending_request(
        &manager->transient_pending_bake_requests,
        &manager->transient_pending_bake_request_count,
        &manager->transient_pending_bake_request_capacity,
        key,
        priority
    )) {
        resource_manager_remove_pending_bake_slot(manager, key);
        return false;
    }
    return true;
}

static uint32_t resource_manager_get_bake_frame_count(
    const VisualSourceResource* source
) {
    uint32_t frame_count;

    if (source == NULL) {
        return 0U;
    }

    frame_count = source->bake_frame_count;
    if (frame_count > 0U && source->animation_fps > 0.0f && source->bake_animation_fps > 0.0f) {
        float loop_duration_seconds = (float)frame_count / source->animation_fps;
        frame_count = (uint32_t)ceilf(loop_duration_seconds * source->bake_animation_fps);
    }
    if (frame_count == 0U) {
        frame_count = source->bake_animation_fps > 0.0f
            ? (uint32_t)ceilf(source->bake_animation_fps)
            : (source->animation_fps > 0.0f ? (uint32_t)ceilf(source->animation_fps) : 1U);
    }
    if (frame_count == 0U) {
        frame_count = 1U;
    }

    return frame_count;
}

static void resource_manager_queue_shared_bake_warmup(
    ResourceManager* manager,
    const VisualSourceResource* source,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t current_frame_index,
    BakedSurfacePass pass
) {
    uint32_t frame_count;
    uint32_t offset;
    BakedSurfaceKey key;

    if (manager == NULL || source == NULL || !source->bake_instance_invariant || source->bake_policy == BAKE_POLICY_DISABLED) {
        return;
    }

    frame_count = resource_manager_get_bake_frame_count(source);
    if (frame_count <= 1U) {
        return;
    }

    for (offset = 1U; offset < frame_count; ++offset) {
        uint32_t next_frame_index = source->loop
            ? ((current_frame_index + offset) % frame_count)
            : (current_frame_index + offset);

        if (!source->loop && next_frame_index >= frame_count) {
            break;
        }

        key.visual_source_id = visual_source_handle.id;
        key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
        key.shader_id = shader_handle.id;
        key.frame_index = next_frame_index;
        key.pass = (uint8_t)pass;
        (void)resource_manager_enqueue_baked_request(manager, key, true);
    }
}

static const Surface* resource_manager_build_baked_surface(
    ResourceManager* manager,
    const VisualSourceResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedSurfaceKey key,
    void* user_data
) {
    size_t index;
    Surface* surface;
    Target target;
    ProceduralTextureContext context;
    Material neutral_material;
    const Material* context_material;

    if (manager == NULL || source == NULL || manager->backend == NULL) {
        return NULL;
    }

    if (manager->bake_budget_per_frame > 0U &&
        manager->bake_requests_this_frame >= manager->bake_budget_per_frame) {
        return NULL;
    }

    if (!resource_manager_reserve_baked_surfaces(manager, manager->baked_surface_count + 1U)) {
        return NULL;
    }

    surface = manager->backend->create_surface(manager->backend->userdata, source->width, source->height);
    if (surface == NULL) {
        return NULL;
    }

    manager->bake_requests_this_frame += 1U;
    manager->backend->set_target(manager->backend->userdata, surface);
    manager->backend->clear(manager->backend->userdata, color_rgba(0U, 0U, 0U, 0U));
    target_init(&target, manager->backend, 0.0f, 0.0f);

    memset(&context, 0, sizeof(context));
    context.now_ms = (uint64_t)(((source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps) > 0.0f)
        ? ((double)key.frame_index / (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps)) * 1000.0
        : 0.0);
    context.time_seconds = (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps) > 0.0f
        ? (float)key.frame_index / (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps)
        : 0.0f;
    context.animation_fps = source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps;
    context.frame_index = key.frame_index;
    context.angle_radians = 0.0f;
    memset(&neutral_material, 0, sizeof(neutral_material));
    neutral_material.base_color = 0xffffffU;
    neutral_material.accent_color = 0xd0d8e8U;
    neutral_material.glow_color = 0xffffffU;
    neutral_material.glare_color = 0xffffffU;
    neutral_material.emissive = 1.0f;
    neutral_material.distortion = 0.0f;
    neutral_material.glare_strength = 1.0f;
    neutral_material.pulse_rate = 1.0f;
    neutral_material.shader_style = shader != NULL
        ? shader->style
        : (material != NULL ? material->value.shader_style : SHADER_STYLE_NONE);
    context_material = source->bake_ignores_material
        ? NULL
        : (material != NULL ? &material->value : &neutral_material);
    context.material = context_material;
    context.shader_style = shader != NULL
        ? shader->style
        : (material != NULL ? material->value.shader_style : neutral_material.shader_style);

    if (key.pass == BAKED_SURFACE_PASS_BODY) {
        source->draw_body(&target, &context, user_data);
    } else {
        source->draw_overlay(&target, &context, user_data);
    }

    manager->backend->reset_target(manager->backend->userdata);

    index = manager->baked_surface_count++;
    manager->baked_surfaces[index].key = key;
    manager->baked_surfaces[index].surface = surface;
    if (!resource_manager_insert_baked_surface_slot(manager, key, index)) {
        manager->backend->destroy_surface(manager->backend->userdata, surface);
        manager->baked_surface_count -= 1U;
        return NULL;
    }
    resource_manager_mark_dirty_baked_surface(manager, key);
    return surface;
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
        manager->bake_cache_hits += 1U;
        return baked_surface;
    }

    manager->bake_cache_misses += 1U;
    (void)user_data;
    return NULL;
}

void resource_manager_request_baked_surface(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedSurfacePass pass
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    BakedSurfaceKey key;
    bool enqueued;
    uint32_t normalized_frame_index;

    if (manager == NULL) {
        return;
    }

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    material = resource_manager_get_material(manager, material_handle);
    if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return;
    }

    key.visual_source_id = visual_source_handle.id;
    key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
    key.shader_id = shader_handle.id;
    normalized_frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.frame_index = normalized_frame_index;
    key.pass = (uint8_t)pass;

    if (visual_source_handle.id > 0U &&
        visual_source_handle.id - 1U < manager->visual_source_last_requested_frame_capacity &&
        manager->visual_source_last_requested_frame_indices[visual_source_handle.id - 1U] != normalized_frame_index) {
        manager->visual_source_last_requested_frame_indices[visual_source_handle.id - 1U] = normalized_frame_index;
        manager->bake_invalidation_animation_frame_count += 1U;
    }
    resource_manager_mark_dirty_visual_source(manager, visual_source_handle);
    if (!source->bake_ignores_material) {
        resource_manager_mark_dirty_material(manager, material_handle);
        manager->prebake_ready_material_count += 1U;
    }
    if (shader_handle.id != 0U) {
        resource_manager_mark_dirty_shader(manager, shader_handle);
    }

    enqueued = resource_manager_enqueue_baked_request(manager, key, source->prebake_required);
    if (enqueued) {
        resource_manager_queue_shared_bake_warmup(
            manager,
            source,
            visual_source_handle,
            material_handle,
            shader_handle,
            key.frame_index,
            pass
        );
    }
}

void resource_manager_request_baked_surface_for_time(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint64_t now_ms,
    BakedSurfacePass pass
) {
    const VisualSourceResource* source;
    uint32_t frame_index = 0U;

    if (manager == NULL) {
        return;
    }

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    if (source != NULL && source->animation_fps > 0.0f) {
        frame_index = (uint32_t)floorf(((float)now_ms / 1000.0f) * source->animation_fps);
    }

    resource_manager_request_baked_surface(
        manager,
        visual_source_handle,
        material_handle,
        shader_handle,
        frame_index,
        pass
    );
}

void resource_manager_process_pending_bakes(ResourceManager* manager) {
    while (manager != NULL &&
           (manager->static_pending_bake_request_head < manager->static_pending_bake_request_count ||
            manager->transient_pending_bake_request_head < manager->transient_pending_bake_request_count) &&
           (manager->bake_budget_per_frame == 0U || manager->bake_requests_this_frame < manager->bake_budget_per_frame)) {
        PendingBakeRequest request;
        BakedSurfaceKey key;
        const VisualSourceResource* source;
        const MaterialResource* material;
        const ShaderResource* shader;

        if (manager->static_pending_bake_request_head < manager->static_pending_bake_request_count) {
            request = manager->static_pending_bake_requests[manager->static_pending_bake_request_head++];
        } else {
            request = manager->transient_pending_bake_requests[manager->transient_pending_bake_request_head++];
        }

        key = request.key;
        source = resource_manager_get_visual_source(manager, resource_handle(key.visual_source_id));
        material = resource_manager_get_material(manager, resource_handle(key.material_id));
        shader = resource_manager_get_shader(manager, resource_handle(key.shader_id));
        resource_manager_remove_pending_bake_slot(manager, key);

        if (source == NULL || !resource_manager_is_bake_eligible(source, (BakedSurfacePass)key.pass)) {
            continue;
        }
        if (!source->bake_ignores_material && material == NULL) {
            continue;
        }
        if (resource_manager_find_baked_surface(manager, key) != NULL) {
            continue;
        }

        (void)resource_manager_build_baked_surface(manager, source, material, shader, key, NULL);
    }

    if (manager != NULL &&
        manager->static_pending_bake_request_head > 0U &&
        manager->static_pending_bake_request_head == manager->static_pending_bake_request_count) {
        manager->static_pending_bake_request_head = 0U;
        manager->static_pending_bake_request_count = 0U;
    }

    if (manager != NULL &&
        manager->transient_pending_bake_request_head > 0U &&
        manager->transient_pending_bake_request_head == manager->transient_pending_bake_request_count) {
        manager->transient_pending_bake_request_head = 0U;
        manager->transient_pending_bake_request_count = 0U;
    }

    if (manager != NULL &&
        manager->static_pending_bake_request_count == 0U &&
        manager->transient_pending_bake_request_count == 0U) {
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

bool resource_manager_prewarm_procedural_source(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    void* user_data
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    const ShaderResource* shader;
    uint32_t frame_count;
    uint32_t frame_index;
    size_t previous_bake_budget;
    BakedSurfaceKey key;

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    material = resource_manager_get_material(manager, material_handle);
    shader = resource_manager_get_shader(manager, shader_handle);
    if (manager == NULL || source == NULL) {
        return false;
    }
    if (!source->bake_ignores_material && material == NULL) {
        return false;
    }
    if (!resource_manager_is_bake_eligible(source, BAKED_SURFACE_PASS_BODY) &&
        !resource_manager_is_bake_eligible(source, BAKED_SURFACE_PASS_OVERLAY)) {
        return false;
    }

    frame_count = resource_manager_get_bake_frame_count(source);

    previous_bake_budget = manager->bake_budget_per_frame;
    manager->bake_budget_per_frame = 0U;

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        if (source->draw_body != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_BODY;
            if (resource_manager_find_baked_surface(manager, key) == NULL &&
                resource_manager_build_baked_surface(manager, source, material, shader, key, user_data) == NULL) {
                manager->bake_budget_per_frame = previous_bake_budget;
                return false;
            }
        }
        if (source->draw_overlay != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_OVERLAY;
            if (resource_manager_find_baked_surface(manager, key) == NULL &&
                resource_manager_build_baked_surface(manager, source, material, shader, key, user_data) == NULL) {
                manager->bake_budget_per_frame = previous_bake_budget;
                return false;
            }
        }
    }

    manager->bake_budget_per_frame = previous_bake_budget;
    return true;
}

size_t resource_manager_queue_required_prebake(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle
) {
    const VisualSourceResource* source;
    uint32_t frame_count;
    uint32_t frame_index;
    size_t enqueued_count = 0U;
    BakedSurfaceKey key;

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    if (manager == NULL || source == NULL || source->bake_policy == BAKE_POLICY_DISABLED || !source->prebake_required) {
        return 0U;
    }

    frame_count = resource_manager_get_bake_frame_count(source);

    for (frame_index = 0U; frame_index < frame_count; ++frame_index) {
        if (source->draw_body != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_BODY;
            if (resource_manager_enqueue_baked_request(manager, key, true)) {
                enqueued_count += 1U;
            }
        }
        if (source->draw_overlay != NULL) {
            key.visual_source_id = visual_source_handle.id;
            key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
            key.shader_id = shader_handle.id;
            key.frame_index = frame_index;
            key.pass = (uint8_t)BAKED_SURFACE_PASS_OVERLAY;
            if (resource_manager_enqueue_baked_request(manager, key, true)) {
                enqueued_count += 1U;
            }
        }
    }

    return enqueued_count;
}

