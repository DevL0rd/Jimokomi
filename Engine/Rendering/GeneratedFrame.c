#include "GeneratedFrame.h"

#include <math.h>
#include <stddef.h>

uint32_t generated_frame_animation_index(const GeneratedFrameConfig* config, uint64_t now_ms)
{
    float time_seconds;

    if (config == NULL || config->animation_fps <= 0.0f)
    {
        return 0U;
    }

    time_seconds = (float)now_ms / 1000.0f;
    return (uint32_t)floorf(time_seconds * config->animation_fps);
}

uint32_t generated_frame_cache_index(const GeneratedFrameConfig* config, uint32_t animation_frame_index)
{
    uint32_t cache_frame_index = animation_frame_index;
    uint32_t cache_frame_count;

    if (config == NULL)
    {
        return animation_frame_index;
    }
    if (config->cache_policy == BAKE_POLICY_REFRESH_FRAME)
    {
        return 0U;
    }

    if (config->animation_fps > 0.0f && config->cache_fps > 0.0f)
    {
        float time_seconds = (float)animation_frame_index / config->animation_fps;
        cache_frame_index = (uint32_t)floorf(time_seconds * config->cache_fps);
    }

    cache_frame_count = generated_frame_cache_frame_count(config);
    if (config->loop && cache_frame_count > 0U)
    {
        return cache_frame_index % cache_frame_count;
    }

    return cache_frame_index;
}

uint32_t generated_frame_bake_source_index(const GeneratedFrameConfig* config, uint32_t animation_frame_index)
{
    if (config == NULL)
    {
        return animation_frame_index;
    }
    if (config->animation_fps > 0.0f && config->cache_fps > 0.0f)
    {
        float time_seconds = (float)animation_frame_index / config->animation_fps;
        return (uint32_t)floorf(time_seconds * config->cache_fps);
    }

    return generated_frame_cache_index(config, animation_frame_index);
}

uint32_t generated_frame_cache_frame_count(const GeneratedFrameConfig* config)
{
    uint32_t frame_count;

    if (config == NULL)
    {
        return 0U;
    }

    frame_count = config->frame_count;
    if (config->cache_policy == BAKE_POLICY_REFRESH_FRAME)
    {
        return 1U;
    }
    if (frame_count > 0U && config->animation_fps > 0.0f && config->cache_fps > 0.0f)
    {
        float loop_duration_seconds = (float)frame_count / config->animation_fps;
        frame_count = (uint32_t)ceilf(loop_duration_seconds * config->cache_fps);
    }
    if (frame_count == 0U)
    {
        frame_count = config->cache_fps > 0.0f
            ? (uint32_t)ceilf(config->cache_fps)
            : (config->animation_fps > 0.0f ? (uint32_t)ceilf(config->animation_fps) : 1U);
    }
    if (frame_count == 0U)
    {
        frame_count = 1U;
    }

    return frame_count;
}

float generated_frame_cache_fps(const GeneratedFrameConfig* config)
{
    if (config == NULL)
    {
        return 0.0f;
    }

    return config->cache_fps > 0.0f ? config->cache_fps : config->animation_fps;
}

float generated_frame_cache_time_seconds(const GeneratedFrameConfig* config, uint32_t cache_frame_index)
{
    float fps = generated_frame_cache_fps(config);

    return fps > 0.0f ? (float)cache_frame_index / fps : 0.0f;
}
