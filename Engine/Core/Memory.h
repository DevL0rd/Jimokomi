#ifndef JIMOKOMI_ENGINE_CORE_MEMORY_H
#define JIMOKOMI_ENGINE_CORE_MEMORY_H

#include <stddef.h>
#include <stdlib.h>

void* engine_memory_malloc(size_t size, const char* name);
void* engine_memory_calloc(size_t count, size_t size, const char* name);
void* engine_memory_realloc(void* memory, size_t size, const char* name);
void engine_memory_free(void* memory, const char* name);

#define malloc(size) engine_memory_malloc((size), "malloc")
#define calloc(count, size) engine_memory_calloc((count), (size), "calloc")
#define realloc(memory, size) engine_memory_realloc((memory), (size), "realloc")
#define free(memory) engine_memory_free((memory), "free")

#endif
