#include "ResourceManagerRegistryInternal.h"

#include "../Core/Memory.h"

#include <stdlib.h>

bool resource_manager_reserve(
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

ResourceHandle resource_handle(uint32_t id) {
    ResourceHandle handle;
    handle.id = id;
    return handle;
}
