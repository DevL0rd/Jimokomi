#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCECOMMANDQUEUE_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCECOMMANDQUEUE_H

#include "ResourceTypes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum ResourceCommandType {
    RESOURCE_COMMAND_REQUEST_BAKED_SURFACE = 0
} ResourceCommandType;

typedef struct ResourceCommand {
    ResourceCommandType type;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    uint64_t now_ms;
    BakedSurfacePass pass;
} ResourceCommand;

typedef struct ResourceManager ResourceManager;
typedef struct ResourceCommandQueue ResourceCommandQueue;

ResourceCommandQueue* resource_command_queue_create(size_t capacity);
void resource_command_queue_destroy(ResourceCommandQueue* queue);
bool resource_command_queue_init(ResourceCommandQueue* queue, size_t capacity);
void resource_command_queue_dispose(ResourceCommandQueue* queue);
bool resource_command_queue_enqueue(ResourceCommandQueue* queue, const ResourceCommand* command);
bool resource_command_queue_has_pending(const ResourceCommandQueue* queue);
size_t resource_command_queue_drain(ResourceCommandQueue* queue, ResourceManager* manager);
uint64_t resource_command_queue_get_dropped_count(const ResourceCommandQueue* queue);
uint64_t resource_command_queue_get_enqueued_count(const ResourceCommandQueue* queue);
uint64_t resource_command_queue_get_drained_count(const ResourceCommandQueue* queue);
uint64_t resource_command_queue_get_deduped_count(const ResourceCommandQueue* queue);

#endif
