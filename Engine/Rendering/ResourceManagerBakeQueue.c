#include "ResourceManagerBakeCacheInternal.h"
#include "ResourceManagerBakeQueueInternal.h"
#include "ResourceManagerInvalidationInternal.h"
#include "ResourceManagerRegistryInternal.h"
#include "ResourceManagerStatsInternal.h"
#include "GeneratedFrame.h"

#include "../Core/Memory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

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

    if (manager->bake_queue->pending_bake_slot_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_queue->pending_bake_slot_capacity > 0U ? manager->bake_queue->pending_bake_slot_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_slots = (PendingBakeSlot*)calloc(next_capacity, sizeof(*next_slots));
    if (next_slots == NULL) {
        return false;
    }

    for (index = 0U; index < manager->bake_queue->pending_bake_slot_capacity; ++index) {
        PendingBakeSlot* slot = &manager->bake_queue->pending_bake_slots[index];
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

    free(manager->bake_queue->pending_bake_slots);
    manager->bake_queue->pending_bake_slots = next_slots;
    manager->bake_queue->pending_bake_slot_capacity = next_capacity;
    return true;
}

static bool resource_manager_pending_bake_contains(
    const ResourceManager* manager,
    BakedTextureKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->bake_queue->pending_bake_slot_capacity == 0U) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_queue->pending_bake_slot_capacity - 1U));
    while (manager->bake_queue->pending_bake_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->bake_queue->pending_bake_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->bake_queue->pending_bake_slots[slot_index].key, key)) {
            return true;
        }
        slot_index = (slot_index + 1U) & (manager->bake_queue->pending_bake_slot_capacity - 1U);
    }

    return false;
}

static bool resource_manager_insert_pending_bake_slot(
    ResourceManager* manager,
    BakedTextureKey key
) {
    size_t slot_index;
    size_t first_tombstone = (size_t)-1;

    if (manager == NULL) {
        return false;
    }

    if ((manager->bake_queue->pending_bake_slot_count + 1U) * 10U >= manager->bake_queue->pending_bake_slot_capacity * 7U) {
        if (!resource_manager_reserve_pending_bake_slots(
                manager,
                manager->bake_queue->pending_bake_slot_capacity > 0U
                    ? manager->bake_queue->pending_bake_slot_capacity * 2U
                    : 64U)) {
            return false;
        }
    }

    if (manager->bake_queue->pending_bake_slot_capacity == 0U &&
        !resource_manager_reserve_pending_bake_slots(manager, 64U)) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_queue->pending_bake_slot_capacity - 1U));
    while (manager->bake_queue->pending_bake_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->bake_queue->pending_bake_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->bake_queue->pending_bake_slots[slot_index].key, key)) {
            return true;
        }
        if (first_tombstone == (size_t)-1 &&
            manager->bake_queue->pending_bake_slots[slot_index].state == SLOT_STATE_TOMBSTONE) {
            first_tombstone = slot_index;
        }
        slot_index = (slot_index + 1U) & (manager->bake_queue->pending_bake_slot_capacity - 1U);
    }

    if (first_tombstone != (size_t)-1) {
        slot_index = first_tombstone;
    }

    manager->bake_queue->pending_bake_slots[slot_index].state = SLOT_STATE_FILLED;
    manager->bake_queue->pending_bake_slots[slot_index].key = key;
    manager->bake_queue->pending_bake_slot_count += 1U;
    return true;
}

void resource_manager_remove_pending_bake_slot(
    ResourceManager* manager,
    BakedTextureKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->bake_queue->pending_bake_slot_capacity == 0U) {
        return;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_queue->pending_bake_slot_capacity - 1U));
    while (manager->bake_queue->pending_bake_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->bake_queue->pending_bake_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->bake_queue->pending_bake_slots[slot_index].key, key)) {
            manager->bake_queue->pending_bake_slots[slot_index].state = SLOT_STATE_TOMBSTONE;
            manager->bake_queue->pending_bake_slot_count -= 1U;
            return;
        }
        slot_index = (slot_index + 1U) & (manager->bake_queue->pending_bake_slot_capacity - 1U);
    }
}

void resource_manager_reset_empty_bake_queue(ResourceManager* manager) {
    if (manager == NULL) {
        return;
    }

    if (manager->bake_queue->static_pending_bake_request_head > 0U &&
        manager->bake_queue->static_pending_bake_request_head == manager->bake_queue->static_pending_bake_request_count) {
        manager->bake_queue->static_pending_bake_request_head = 0U;
        manager->bake_queue->static_pending_bake_request_count = 0U;
    }

    if (manager->bake_queue->transient_pending_bake_request_head > 0U &&
        manager->bake_queue->transient_pending_bake_request_head == manager->bake_queue->transient_pending_bake_request_count) {
        manager->bake_queue->transient_pending_bake_request_head = 0U;
        manager->bake_queue->transient_pending_bake_request_count = 0U;
    }

    if ((manager->bake_queue->static_pending_bake_request_count == 0U ||
         manager->bake_queue->static_pending_bake_request_head == manager->bake_queue->static_pending_bake_request_count) &&
        (manager->bake_queue->transient_pending_bake_request_count == 0U ||
         manager->bake_queue->transient_pending_bake_request_head == manager->bake_queue->transient_pending_bake_request_count)) {
        manager->bake_queue->pending_bake_slot_count = 0U;
        if (manager->bake_queue->pending_bake_slots != NULL && manager->bake_queue->pending_bake_slot_capacity > 0U) {
            memset(
                manager->bake_queue->pending_bake_slots,
                0,
                manager->bake_queue->pending_bake_slot_capacity * sizeof(*manager->bake_queue->pending_bake_slots)
            );
        }
    }
}

static bool resource_manager_reserve_bake_interest_entries(
    ResourceManager* manager,
    size_t required
) {
    BakeInterestEntry* next_values;
    size_t next_capacity;

    if (manager->bake_queue->bake_interest_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_queue->bake_interest_capacity > 0U ? manager->bake_queue->bake_interest_capacity : 32U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakeInterestEntry*)realloc(
        manager->bake_queue->bake_interest_entries,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->bake_queue->bake_interest_entries = next_values;
    manager->bake_queue->bake_interest_capacity = next_capacity;
    return true;
}

static size_t resource_manager_get_bake_interest_priority(
    const ResourceManager* manager,
    BakedTextureKey key
) {
    size_t index;

    if (manager == NULL) {
        return 0U;
    }

    for (index = 0U; index < manager->bake_queue->bake_interest_count; ++index) {
        if (resource_manager_baked_key_equals(manager->bake_queue->bake_interest_entries[index].key, key)) {
            return (size_t)manager->bake_queue->bake_interest_entries[index].frame_hits * 4U +
                   (size_t)manager->bake_queue->bake_interest_entries[index].total_hits;
        }
    }

    return 0U;
}

static bool resource_manager_enqueue_pending_request(
    PendingBakeRequest** requests,
    size_t* count,
    size_t* capacity,
    BakedTextureKey key,
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

static bool resource_manager_should_admit_bake(
    ResourceManager* manager,
    BakedTextureKey key
) {
    size_t index;
    BakeInterestEntry* entry;

    if (manager == NULL) {
        return false;
    }

    for (index = 0U; index < manager->bake_queue->bake_interest_count; ++index) {
        if (resource_manager_baked_key_equals(manager->bake_queue->bake_interest_entries[index].key, key)) {
            entry = &manager->bake_queue->bake_interest_entries[index];
            if (entry->last_seen_frame != manager->bake_queue->frame_serial) {
                entry->frame_hits = 0U;
                entry->last_seen_frame = manager->bake_queue->frame_serial;
            }
            entry->frame_hits += 1U;
            entry->total_hits += 1U;
            return entry->frame_hits >= manager->bake_queue->bake_admission_frame_hits ||
                   entry->total_hits >= manager->bake_queue->bake_admission_total_hits;
        }
    }

    if (!resource_manager_reserve_bake_interest_entries(manager, manager->bake_queue->bake_interest_count + 1U)) {
        return false;
    }

    entry = &manager->bake_queue->bake_interest_entries[manager->bake_queue->bake_interest_count++];
    memset(entry, 0, sizeof(*entry));
    entry->key = key;
    entry->total_hits = 1U;
    entry->frame_hits = 1U;
    entry->last_seen_frame = manager->bake_queue->frame_serial;
    return entry->frame_hits >= manager->bake_queue->bake_admission_frame_hits ||
           entry->total_hits >= manager->bake_queue->bake_admission_total_hits;
}

bool resource_manager_enqueue_baked_request(
    ResourceManager* manager,
    BakedTextureKey key,
    bool bypass_admission
) {
    const ProceduralTextureResource* source;
    uint32_t priority;

    if (manager == NULL) {
        return false;
    }
    if (resource_manager_find_baked_texture(manager, key) != NULL ||
        resource_manager_pending_bake_contains(manager, key)) {
        return false;
    }
    if (!bypass_admission && !resource_manager_should_admit_bake(manager, key)) {
        return false;
    }
    if (!resource_manager_insert_pending_bake_slot(manager, key)) {
        return false;
    }

    source = resource_manager_get_procedural_texture(manager, resource_handle(key.procedural_texture_id));
    priority = (uint32_t)resource_manager_get_bake_interest_priority(manager, key);
    if (source != NULL && source->frames.prebake_enabled) {
        priority += 1000000U;
        if (!resource_manager_enqueue_pending_request(
            &manager->bake_queue->static_pending_bake_requests,
            &manager->bake_queue->static_pending_bake_request_count,
            &manager->bake_queue->static_pending_bake_request_capacity,
            key,
            priority
        )) {
            resource_manager_remove_pending_bake_slot(manager, key);
            return false;
        }
        return true;
    }

    if (!resource_manager_enqueue_pending_request(
        &manager->bake_queue->transient_pending_bake_requests,
        &manager->bake_queue->transient_pending_bake_request_count,
        &manager->bake_queue->transient_pending_bake_request_capacity,
        key,
        priority
    )) {
        resource_manager_remove_pending_bake_slot(manager, key);
        return false;
    }
    return true;
}

static void resource_manager_queue_shared_bake_warmup(
    ResourceManager* manager,
    const ProceduralTextureResource* source,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t current_frame_index,
    BakedTexturePass pass
) {
    uint32_t frame_count;
    uint32_t offset;
    BakedTextureKey key;

    if (manager == NULL || source == NULL || !source->frames.instance_invariant ||
        source->frames.cache_policy == BAKE_POLICY_DISABLED ||
        source->frames.cache_policy == BAKE_POLICY_REFRESH_FRAME) {
        return;
    }

    frame_count = resource_manager_get_bake_frame_count(source);
    if (frame_count <= 1U) {
        return;
    }

    for (offset = 1U; offset < frame_count; ++offset) {
        uint32_t next_frame_index = source->frames.loop
            ? ((current_frame_index + offset) % frame_count)
            : (current_frame_index + offset);

        if (!source->frames.loop && next_frame_index >= frame_count) {
            break;
        }

        key.procedural_texture_id = procedural_texture_handle.id;
        key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
        key.shader_id = shader_handle.id;
        key.frame_index = next_frame_index;
        key.pass = (uint8_t)pass;
        (void)resource_manager_enqueue_baked_request(manager, key, true);
    }
}

void resource_manager_request_baked_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedTexturePass pass
) {
    const ProceduralTextureResource* source;
    const MaterialResource* material;
    BakedTextureKey key;
    bool enqueued;
    uint32_t normalized_frame_index;

    if (manager == NULL) {
        return;
    }

    source = resource_manager_get_procedural_texture(manager, procedural_texture_handle);
    material = resource_manager_get_material(manager, material_handle);
    if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return;
    }
    if (source->frames.cache_policy == BAKE_POLICY_REFRESH_FRAME) {
        return;
    }

    key.procedural_texture_id = procedural_texture_handle.id;
    key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
    key.shader_id = shader_handle.id;
    normalized_frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.frame_index = normalized_frame_index;
    key.pass = (uint8_t)pass;

    if (procedural_texture_handle.id > 0U &&
        procedural_texture_handle.id - 1U < manager->bake_queue->procedural_texture_last_requested_frame_capacity &&
        manager->bake_queue->procedural_texture_last_requested_frame_indices[procedural_texture_handle.id - 1U] != normalized_frame_index) {
        manager->bake_queue->procedural_texture_last_requested_frame_indices[procedural_texture_handle.id - 1U] = normalized_frame_index;
        manager->stats->bake_invalidation_animation_frame_count += 1U;
    }
    resource_manager_mark_dirty_procedural_texture(manager, procedural_texture_handle);
    if (!source->bake_ignores_material) {
        resource_manager_mark_dirty_material(manager, material_handle);
        manager->stats->prebake_ready_material_count += 1U;
    }
    if (shader_handle.id != 0U) {
        resource_manager_mark_dirty_shader(manager, shader_handle);
    }

    enqueued = resource_manager_enqueue_baked_request(manager, key, source->frames.prebake_enabled);
    if (enqueued) {
        resource_manager_queue_shared_bake_warmup(
            manager,
            source,
            procedural_texture_handle,
            material_handle,
            shader_handle,
            key.frame_index,
            pass
        );
    }
}

void resource_manager_request_baked_texture_for_time(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint64_t now_ms,
    BakedTexturePass pass
) {
    const ProceduralTextureResource* source;
    uint32_t frame_index = 0U;

    if (manager == NULL) {
        return;
    }

    source = resource_manager_get_procedural_texture(manager, procedural_texture_handle);
    frame_index = generated_frame_animation_index(source != NULL ? &source->frames : NULL, now_ms);

    resource_manager_request_baked_texture(
        manager,
        procedural_texture_handle,
        material_handle,
        shader_handle,
        frame_index,
        pass
    );
}
