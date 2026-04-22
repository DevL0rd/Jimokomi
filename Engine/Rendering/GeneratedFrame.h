#ifndef JIMOKOMI_ENGINE_RENDERING_GENERATEDFRAME_H
#define JIMOKOMI_ENGINE_RENDERING_GENERATEDFRAME_H

#include "ResourceTypes.h"

#include <stdint.h>

uint32_t generated_frame_animation_index(const GeneratedFrameConfig* config, uint64_t now_ms);
uint32_t generated_frame_cache_index(const GeneratedFrameConfig* config, uint32_t animation_frame_index);
uint32_t generated_frame_bake_source_index(const GeneratedFrameConfig* config, uint32_t animation_frame_index);
uint32_t generated_frame_cache_frame_count(const GeneratedFrameConfig* config);
float generated_frame_cache_fps(const GeneratedFrameConfig* config);
float generated_frame_cache_time_seconds(const GeneratedFrameConfig* config, uint32_t cache_frame_index);

#endif
