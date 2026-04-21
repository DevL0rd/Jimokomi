#include "InputPacketStreamInternal.h"

#include <stdlib.h>
#include <string.h>

bool input_packet_stream_init(InputPacketStream* stream, size_t packet_size) {
    if (stream == NULL || packet_size == 0U) {
        return false;
    }

    memset(stream, 0, sizeof(*stream));
    stream->packets = calloc(2U, packet_size);
    if (stream->packets == NULL) {
        return false;
    }
    stream->packet_size = packet_size;
    atomic_init(&stream->published_index, 0U);
    atomic_init(&stream->published_frame_id, 0U);
    return true;
}

void input_packet_stream_dispose(InputPacketStream* stream) {
    if (stream == NULL) {
        return;
    }

    free(stream->packets);
    memset(stream, 0, sizeof(*stream));
}

void* input_packet_stream_begin_write(InputPacketStream* stream) {
    unsigned char* packets;
    void* packet;

    if (stream == NULL || stream->packets == NULL || stream->packet_size == 0U) {
        return NULL;
    }

    packets = (unsigned char*)stream->packets;
    packet = packets + (stream->write_index * stream->packet_size);
    memset(packet, 0, stream->packet_size);
    return packet;
}

uint64_t input_packet_stream_next_frame_id(const InputPacketStream* stream) {
    if (stream == NULL) {
        return 0U;
    }

    return atomic_load_explicit(&stream->published_frame_id, memory_order_relaxed) + 1U;
}

void input_packet_stream_publish(InputPacketStream* stream, void* packet, uint64_t frame_id) {
    unsigned char* packets;
    uintptr_t offset;
    size_t index;

    if (stream == NULL || stream->packets == NULL || packet == NULL || stream->packet_size == 0U) {
        return;
    }

    packets = (unsigned char*)stream->packets;
    offset = (uintptr_t)((unsigned char*)packet - packets);
    index = offset >= stream->packet_size ? 1U : 0U;
    if (frame_id == 0U) {
        frame_id = input_packet_stream_next_frame_id(stream);
    }
    atomic_store_explicit(&stream->published_index, index, memory_order_release);
    atomic_store_explicit(&stream->published_frame_id, frame_id, memory_order_release);
    stream->write_index = 1U - index;
}

bool input_packet_stream_read_latest(const InputPacketStream* stream, void* packet) {
    const unsigned char* packets;
    uint64_t frame_id;
    size_t index;

    if (stream == NULL || stream->packets == NULL || packet == NULL || stream->packet_size == 0U) {
        return false;
    }

    frame_id = atomic_load_explicit(&stream->published_frame_id, memory_order_acquire);
    if (frame_id == 0U) {
        return false;
    }

    index = atomic_load_explicit(&stream->published_index, memory_order_acquire);
    if (index > 1U) {
        return false;
    }

    packets = (const unsigned char*)stream->packets;
    memcpy(packet, packets + (index * stream->packet_size), stream->packet_size);
    return true;
}
