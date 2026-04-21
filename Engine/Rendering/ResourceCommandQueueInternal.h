#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCECOMMANDQUEUE_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCECOMMANDQUEUE_INTERNAL_H

#include "ResourceCommandQueue.h"

#include <stdatomic.h>
#include <stdint.h>

typedef struct ResourceCommandSlot {
    uint8_t state;
    ResourceCommandType type;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    BakedSurfacePass pass;
} ResourceCommandSlot;

struct ResourceCommandQueue {
    ResourceCommand* commands;
    size_t capacity;
    ResourceCommandSlot* slots;
    size_t slot_capacity;
    size_t slot_count;
    atomic_size_t head;
    atomic_size_t tail;
    atomic_uint_fast64_t dropped_count;
    atomic_uint_fast64_t enqueued_count;
    atomic_uint_fast64_t drained_count;
    atomic_uint_fast64_t deduped_count;
};

#endif
