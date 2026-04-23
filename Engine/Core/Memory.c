#include "Profiling.h"

#include <stdint.h>
#include <stdlib.h>

static const char* engine_memory_profile_name(void)
{
    return "Engine Heap";
}

void* engine_memory_malloc(size_t size, const char* name)
{
    void* memory = NULL;

    if (size == 0U)
    {
        return NULL;
    }

    memory = malloc(size);
    engine_profile_memory_alloc(memory, size, engine_memory_profile_name(), 4);
    return memory;
}

void* engine_memory_calloc(size_t count, size_t size, const char* name)
{
    void* memory = NULL;
    size_t total_size = count * size;

    if (count == 0U || size == 0U)
    {
        return NULL;
    }

    memory = calloc(count, size);
    engine_profile_memory_alloc(memory, total_size, engine_memory_profile_name(), 4);
    return memory;
}

void* engine_memory_realloc(void* memory, size_t size, const char* name)
{
    void* resized_memory = NULL;
    uintptr_t previous_memory = (uintptr_t)memory;

    if (memory == NULL)
    {
        return engine_memory_malloc(size, name);
    }

    if (size == 0U)
    {
        engine_profile_memory_free(memory, engine_memory_profile_name(), 4);
        free(memory);
        return NULL;
    }

    resized_memory = realloc(memory, size);
    if (resized_memory == NULL)
    {
        return NULL;
    }

    engine_profile_memory_free((const void*)previous_memory, engine_memory_profile_name(), 4);
    engine_profile_memory_alloc(resized_memory, size, engine_memory_profile_name(), 4);
    return resized_memory;
}

void engine_memory_free(void* memory, const char* name)
{
    if (memory == NULL)
    {
        return;
    }

    engine_profile_memory_free(memory, engine_memory_profile_name(), 4);
    free(memory);
}
