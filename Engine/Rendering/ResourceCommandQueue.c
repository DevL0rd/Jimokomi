#include "ResourceCommandQueueInternal.h"

#include "ResourceManager.h"

#include <stdlib.h>
#include <string.h>

enum {
    RESOURCE_COMMAND_SLOT_EMPTY = 0,
    RESOURCE_COMMAND_SLOT_FILLED = 1,
    RESOURCE_COMMAND_SLOT_TOMBSTONE = 2
};

static uint64_t resource_command_queue_hash(
    ResourceCommandType type,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    BakedSurfacePass pass
) {
    uint64_t hash = 1469598103934665603ULL;
    hash ^= (uint64_t)type;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)visual_source_handle.id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)material_handle.id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)shader_handle.id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)pass;
    hash *= 1099511628211ULL;
    return hash;
}

static bool resource_command_queue_slot_equals(
    const ResourceCommandSlot* slot,
    const ResourceCommand* command
) {
    return slot != NULL &&
           command != NULL &&
           slot->type == command->type &&
           slot->visual_source_handle.id == command->visual_source_handle.id &&
           slot->material_handle.id == command->material_handle.id &&
           slot->shader_handle.id == command->shader_handle.id &&
           slot->pass == command->pass;
}

static bool resource_command_queue_reserve_slots(ResourceCommandQueue* queue, size_t required_capacity) {
    ResourceCommandSlot* next_slots;
    size_t next_capacity;
    size_t index;

    if (queue == NULL) {
        return false;
    }

    if (queue->slot_capacity >= required_capacity) {
        return true;
    }

    next_capacity = queue->slot_capacity > 0U ? queue->slot_capacity : 64U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_slots = (ResourceCommandSlot*)calloc(next_capacity, sizeof(*next_slots));
    if (next_slots == NULL) {
        return false;
    }

    for (index = 0U; index < queue->slot_capacity; ++index) {
        ResourceCommandSlot* slot = &queue->slots[index];
        size_t slot_index;

        if (slot->state != RESOURCE_COMMAND_SLOT_FILLED) {
            continue;
        }

        slot_index = (size_t)(resource_command_queue_hash(
            slot->type,
            slot->visual_source_handle,
            slot->material_handle,
            slot->shader_handle,
            slot->pass
        ) & (uint64_t)(next_capacity - 1U));
        while (next_slots[slot_index].state == RESOURCE_COMMAND_SLOT_FILLED) {
            slot_index = (slot_index + 1U) & (next_capacity - 1U);
        }
        next_slots[slot_index] = *slot;
    }

    free(queue->slots);
    queue->slots = next_slots;
    queue->slot_capacity = next_capacity;
    return true;
}

static bool resource_command_queue_contains_locked(const ResourceCommandQueue* queue, const ResourceCommand* command) {
    size_t slot_index;

    if (queue == NULL || command == NULL || queue->slot_capacity == 0U) {
        return false;
    }

    slot_index = (size_t)(resource_command_queue_hash(
        command->type,
        command->visual_source_handle,
        command->material_handle,
        command->shader_handle,
        command->pass
    ) & (uint64_t)(queue->slot_capacity - 1U));
    while (queue->slots[slot_index].state != RESOURCE_COMMAND_SLOT_EMPTY) {
        if (queue->slots[slot_index].state == RESOURCE_COMMAND_SLOT_FILLED &&
            resource_command_queue_slot_equals(&queue->slots[slot_index], command)) {
            return true;
        }
        slot_index = (slot_index + 1U) & (queue->slot_capacity - 1U);
    }
    return false;
}

static bool resource_command_queue_insert_locked(ResourceCommandQueue* queue, const ResourceCommand* command) {
    size_t slot_index;
    size_t first_tombstone = (size_t)-1;

    if (queue == NULL || command == NULL) {
        return false;
    }

    if ((queue->slot_count + 1U) * 10U >= queue->slot_capacity * 7U) {
        if (!resource_command_queue_reserve_slots(
                queue,
                queue->slot_capacity > 0U ? queue->slot_capacity * 2U : 64U)) {
            return false;
        }
    }

    if (queue->slot_capacity == 0U &&
        !resource_command_queue_reserve_slots(queue, 64U)) {
        return false;
    }

    slot_index = (size_t)(resource_command_queue_hash(
        command->type,
        command->visual_source_handle,
        command->material_handle,
        command->shader_handle,
        command->pass
    ) & (uint64_t)(queue->slot_capacity - 1U));
    while (queue->slots[slot_index].state != RESOURCE_COMMAND_SLOT_EMPTY) {
        if (queue->slots[slot_index].state == RESOURCE_COMMAND_SLOT_FILLED &&
            resource_command_queue_slot_equals(&queue->slots[slot_index], command)) {
            return true;
        }
        if (first_tombstone == (size_t)-1 &&
            queue->slots[slot_index].state == RESOURCE_COMMAND_SLOT_TOMBSTONE) {
            first_tombstone = slot_index;
        }
        slot_index = (slot_index + 1U) & (queue->slot_capacity - 1U);
    }

    if (first_tombstone != (size_t)-1) {
        slot_index = first_tombstone;
    }

    queue->slots[slot_index].state = RESOURCE_COMMAND_SLOT_FILLED;
    queue->slots[slot_index].type = command->type;
    queue->slots[slot_index].visual_source_handle = command->visual_source_handle;
    queue->slots[slot_index].material_handle = command->material_handle;
    queue->slots[slot_index].shader_handle = command->shader_handle;
    queue->slots[slot_index].pass = command->pass;
    queue->slot_count += 1U;
    return true;
}

static void resource_command_queue_remove_locked(ResourceCommandQueue* queue, const ResourceCommand* command) {
    size_t slot_index;

    if (queue == NULL || command == NULL || queue->slot_capacity == 0U) {
        return;
    }

    slot_index = (size_t)(resource_command_queue_hash(
        command->type,
        command->visual_source_handle,
        command->material_handle,
        command->shader_handle,
        command->pass
    ) & (uint64_t)(queue->slot_capacity - 1U));
    while (queue->slots[slot_index].state != RESOURCE_COMMAND_SLOT_EMPTY) {
        if (queue->slots[slot_index].state == RESOURCE_COMMAND_SLOT_FILLED &&
            resource_command_queue_slot_equals(&queue->slots[slot_index], command)) {
            queue->slots[slot_index].state = RESOURCE_COMMAND_SLOT_TOMBSTONE;
            if (queue->slot_count > 0U) {
                queue->slot_count -= 1U;
            }
            return;
        }
        slot_index = (slot_index + 1U) & (queue->slot_capacity - 1U);
    }
}

bool resource_command_queue_init(ResourceCommandQueue* queue, size_t capacity) {
    if (queue == NULL || capacity < 2U) {
        return false;
    }

    memset(queue, 0, sizeof(*queue));
    queue->commands = (ResourceCommand*)calloc(capacity, sizeof(*queue->commands));
    if (queue->commands == NULL) {
        return false;
    }

    queue->capacity = capacity;
    queue->slots = NULL;
    queue->slot_capacity = 0U;
    queue->slot_count = 0U;
    atomic_init(&queue->head, 0U);
    atomic_init(&queue->tail, 0U);
    atomic_init(&queue->dropped_count, 0U);
    atomic_init(&queue->enqueued_count, 0U);
    atomic_init(&queue->drained_count, 0U);
    atomic_init(&queue->deduped_count, 0U);
    return true;
}

ResourceCommandQueue* resource_command_queue_create(size_t capacity) {
    ResourceCommandQueue* queue = (ResourceCommandQueue*)calloc(1, sizeof(*queue));
    if (queue == NULL) {
        return NULL;
    }

    if (!resource_command_queue_init(queue, capacity)) {
        free(queue);
        return NULL;
    }

    return queue;
}

void resource_command_queue_dispose(ResourceCommandQueue* queue) {
    if (queue == NULL) {
        return;
    }

    free(queue->commands);
    free(queue->slots);
    memset(queue, 0, sizeof(*queue));
}

void resource_command_queue_destroy(ResourceCommandQueue* queue) {
    if (queue == NULL) {
        return;
    }

    resource_command_queue_dispose(queue);
    free(queue);
}

bool resource_command_queue_enqueue(ResourceCommandQueue* queue, const ResourceCommand* command) {
    size_t tail;
    size_t next_tail;
    size_t head;

    if (queue == NULL || queue->commands == NULL || command == NULL) {
        return false;
    }

    tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    next_tail = (tail + 1U) % queue->capacity;
    head = atomic_load_explicit(&queue->head, memory_order_acquire);
    if (resource_command_queue_contains_locked(queue, command)) {
        atomic_fetch_add_explicit(&queue->deduped_count, 1U, memory_order_relaxed);
        return true;
    }
    if (next_tail == head) {
        atomic_fetch_add_explicit(&queue->dropped_count, 1U, memory_order_relaxed);
        return false;
    }

    if (!resource_command_queue_insert_locked(queue, command)) {
        atomic_fetch_add_explicit(&queue->dropped_count, 1U, memory_order_relaxed);
        return false;
    }

    queue->commands[tail] = *command;
    atomic_store_explicit(&queue->tail, next_tail, memory_order_release);
    atomic_fetch_add_explicit(&queue->enqueued_count, 1U, memory_order_relaxed);
    return true;
}

bool resource_command_queue_has_pending(const ResourceCommandQueue* queue) {
    if (queue == NULL || queue->commands == NULL) {
        return false;
    }

    return atomic_load_explicit(&queue->head, memory_order_acquire) !=
           atomic_load_explicit(&queue->tail, memory_order_acquire);
}

size_t resource_command_queue_drain(ResourceCommandQueue* queue, ResourceManager* manager) {
    size_t drained = 0U;

    if (queue == NULL || manager == NULL || queue->commands == NULL) {
        return 0U;
    }

    for (;;) {
        size_t head = atomic_load_explicit(&queue->head, memory_order_relaxed);
        size_t tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
        ResourceCommand command;

        if (head == tail) {
            break;
        }

        command = queue->commands[head];
        resource_command_queue_remove_locked(queue, &command);
        atomic_store_explicit(&queue->head, (head + 1U) % queue->capacity, memory_order_release);
        drained += 1U;

        if (command.type == RESOURCE_COMMAND_REQUEST_BAKED_SURFACE) {
            resource_manager_request_baked_surface_for_time(
                manager,
                command.visual_source_handle,
                command.material_handle,
                command.shader_handle,
                command.now_ms,
                command.pass
            );
        }
    }

    atomic_fetch_add_explicit(&queue->drained_count, drained, memory_order_relaxed);
    return drained;
}

uint64_t resource_command_queue_get_dropped_count(const ResourceCommandQueue* queue) {
    return queue != NULL ? atomic_load_explicit(&queue->dropped_count, memory_order_relaxed) : 0U;
}

uint64_t resource_command_queue_get_enqueued_count(const ResourceCommandQueue* queue) {
    return queue != NULL ? atomic_load_explicit(&queue->enqueued_count, memory_order_relaxed) : 0U;
}

uint64_t resource_command_queue_get_drained_count(const ResourceCommandQueue* queue) {
    return queue != NULL ? atomic_load_explicit(&queue->drained_count, memory_order_relaxed) : 0U;
}

uint64_t resource_command_queue_get_deduped_count(const ResourceCommandQueue* queue) {
    return queue != NULL ? atomic_load_explicit(&queue->deduped_count, memory_order_relaxed) : 0U;
}
