#ifndef JIMOKOMI_ENGINE_CORE_INPUTPACKETSTREAM_H
#define JIMOKOMI_ENGINE_CORE_INPUTPACKETSTREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct InputPacketStream InputPacketStream;

bool input_packet_stream_init(InputPacketStream* stream, size_t packet_size);
void input_packet_stream_dispose(InputPacketStream* stream);
void* input_packet_stream_begin_write(InputPacketStream* stream);
uint64_t input_packet_stream_next_frame_id(const InputPacketStream* stream);
void input_packet_stream_publish(InputPacketStream* stream, void* packet, uint64_t frame_id);
bool input_packet_stream_read_latest(const InputPacketStream* stream, void* packet);

#endif
