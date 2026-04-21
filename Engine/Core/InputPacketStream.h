#ifndef JIMOKOMI_ENGINE_CORE_INPUTPACKETSTREAM_H
#define JIMOKOMI_ENGINE_CORE_INPUTPACKETSTREAM_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct InputPacketStream {
    void* packets;
    size_t packet_size;
    atomic_size_t published_index;
    atomic_uint_fast64_t published_frame_id;
    size_t write_index;
} InputPacketStream;

bool input_packet_stream_init(InputPacketStream* stream, size_t packet_size);
void input_packet_stream_dispose(InputPacketStream* stream);
void* input_packet_stream_begin_write(InputPacketStream* stream);
uint64_t input_packet_stream_next_frame_id(const InputPacketStream* stream);
void input_packet_stream_publish(InputPacketStream* stream, void* packet, uint64_t frame_id);
bool input_packet_stream_read_latest(const InputPacketStream* stream, void* packet);

#endif
