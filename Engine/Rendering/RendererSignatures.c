#include "RendererInternal.h"

#include "RendererSignaturesInternal.h"

static uint64_t renderer_hash_u64(uint64_t hash, uint64_t value)
{
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    return hash;
}

uint64_t renderer_compute_overlay_signature(const RendererFrame* frame)
{
    uint64_t hash = 2166136261ULL;

    if (frame == NULL)
    {
        return 0U;
    }

    hash = renderer_hash_u64(hash, frame->debug_entity_count);
    hash = renderer_hash_u64(hash, frame->debug_collision_count);
    hash = renderer_hash_u64(hash, frame->selected_debug_entity_id);
    hash = renderer_hash_u64(hash, frame->hovered_debug_entity_id);
    hash = renderer_hash_u64(hash, frame->draw_debug ? 1U : 0U);
    return hash;
}

void renderer_compute_frame_signatures(
    const RendererFrame* frame,
    uint64_t* out_frame_signature,
    uint64_t* out_sort_signature,
    uint64_t* out_instance_signature,
    bool* out_procedural_textures_sorted_by_layer
)
{
    uint64_t frame_hash = 1469598103934665603ULL;
    uint64_t sort_hash = 1099511628211ULL;
    uint64_t instance_hash = 88172645463325252ULL;
    size_t index = 0U;
    int previous_layer = 0;
    bool procedural_textures_sorted_by_layer = true;

    if (frame == NULL)
    {
        if (out_frame_signature != NULL)
        {
            *out_frame_signature = 0U;
        }
        if (out_sort_signature != NULL)
        {
            *out_sort_signature = 0U;
        }
        if (out_instance_signature != NULL)
        {
            *out_instance_signature = 0U;
        }
        if (out_procedural_textures_sorted_by_layer != NULL)
        {
            *out_procedural_textures_sorted_by_layer = true;
        }
        return;
    }

    frame_hash = renderer_hash_u64(frame_hash, frame->procedural_texture_count);
    frame_hash = renderer_hash_u64(frame_hash, frame->draw_scene_renderables ? 1U : 0U);
    frame_hash = renderer_hash_u64(frame_hash, frame->draw_debug ? 1U : 0U);
    frame_hash = renderer_hash_u64(frame_hash, frame->metadata_signature);
    if (frame->procedural_texture_signatures_valid)
    {
        frame_hash = renderer_hash_u64(frame_hash, frame->procedural_texture_frame_signature);
        sort_hash = renderer_hash_u64(sort_hash, frame->procedural_texture_sort_signature);
        instance_hash = renderer_hash_u64(instance_hash, frame->procedural_texture_instance_signature);
        procedural_textures_sorted_by_layer = frame->procedural_textures_sorted_by_layer;
    }
    else
    {
        sort_hash = renderer_hash_u64(sort_hash, frame->procedural_texture_count);
        instance_hash = renderer_hash_u64(instance_hash, frame->procedural_texture_count);
        for (index = 0U; index < frame->procedural_texture_count; ++index)
        {
            const ProceduralTextureRenderable* item = &frame->procedural_textures[index];

            if (index > 0U && item->layer < previous_layer)
            {
                procedural_textures_sorted_by_layer = false;
            }
            previous_layer = item->layer;
            frame_hash = renderer_hash_u64(frame_hash, (uint64_t)(int64_t)item->layer);
            frame_hash = renderer_hash_u64(frame_hash, item->visible ? 1U : 0U);
            frame_hash = renderer_hash_u64(frame_hash, item->texture_handle.id);
            frame_hash = renderer_hash_u64(frame_hash, item->procedural_texture_handle.id);
            frame_hash = renderer_hash_u64(frame_hash, item->material_handle.id);
            frame_hash = renderer_hash_u64(frame_hash, item->shader_handle.id);
            frame_hash = renderer_hash_u64(frame_hash, item->tint.value);
            sort_hash = renderer_hash_u64(sort_hash, (uint64_t)(int64_t)item->layer);
            instance_hash = renderer_hash_u64(instance_hash, item->texture_handle.id);
            instance_hash = renderer_hash_u64(instance_hash, item->procedural_texture_handle.id);
            instance_hash = renderer_hash_u64(instance_hash, item->material_handle.id);
            instance_hash = renderer_hash_u64(instance_hash, item->shader_handle.id);
            instance_hash = renderer_hash_u64(instance_hash, item->tint.value);
        }
    }

    if (out_frame_signature != NULL)
    {
        *out_frame_signature = frame_hash;
    }
    if (out_sort_signature != NULL)
    {
        *out_sort_signature = sort_hash;
    }
    if (out_instance_signature != NULL)
    {
        *out_instance_signature = instance_hash;
    }
    if (out_procedural_textures_sorted_by_layer != NULL)
    {
        *out_procedural_textures_sorted_by_layer = procedural_textures_sorted_by_layer;
    }
}
