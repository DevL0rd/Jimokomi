#ifndef JIMOKOMI_ENGINE_CORE_INPUTPACKETSTREAM_INTERNAL_H
#define JIMOKOMI_ENGINE_CORE_INPUTPACKETSTREAM_INTERNAL_H

#include "InputPacketStream.h"

#include <stdatomic.h>

struct InputPacketStream {
    void* packets;
    size_t packet_size;
    atomic_size_t published_index;
    atomic_uint_fast64_t published_frame_id;
    size_t write_index;
};

#endif
