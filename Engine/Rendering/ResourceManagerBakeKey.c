#include "ResourceManagerInternal.h"

#include <math.h>

uint64_t resource_manager_hash_baked_key(BakedSurfaceKey key) {
    uint64_t hash = 1469598103934665603ULL;
    hash ^= (uint64_t)key.visual_source_id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.material_id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.shader_id;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.frame_index;
    hash *= 1099511628211ULL;
    hash ^= (uint64_t)key.pass;
    hash *= 1099511628211ULL;
    return hash;
}

bool resource_manager_baked_key_equals(BakedSurfaceKey a, BakedSurfaceKey b) {
    return a.visual_source_id == b.visual_source_id &&
           a.material_id == b.material_id &&
           a.shader_id == b.shader_id &&
           a.frame_index == b.frame_index &&
           a.pass == b.pass;
}

uint32_t resource_manager_normalize_frame_index(
    const VisualSourceResource* source,
    uint32_t frame_index
) {
    uint32_t baked_frame_index = frame_index;
    uint32_t baked_frame_count;

    if (source == NULL) {
        return frame_index;
    }

    if (source->animation_fps > 0.0f && source->bake_animation_fps > 0.0f) {
        float time_seconds = (float)frame_index / source->animation_fps;
        baked_frame_index = (uint32_t)floorf(time_seconds * source->bake_animation_fps);
    }

    baked_frame_count = 0U;
    if (source->bake_frame_count > 0U) {
        if (source->animation_fps > 0.0f && source->bake_animation_fps > 0.0f) {
            float loop_duration_seconds = (float)source->bake_frame_count / source->animation_fps;
            baked_frame_count = (uint32_t)ceilf(loop_duration_seconds * source->bake_animation_fps);
        } else {
            baked_frame_count = source->bake_frame_count;
        }
    }

    if (source->loop && baked_frame_count > 0U) {
        return baked_frame_index % baked_frame_count;
    }
    return baked_frame_index;
}

bool resource_manager_is_bake_eligible(
    const VisualSourceResource* source,
    BakedSurfacePass pass
) {
    if (source == NULL) {
        return false;
    }
    if (source->bake_policy == BAKE_POLICY_DISABLED || !source->bake_instance_invariant) {
        return false;
    }
    if (pass == BAKED_SURFACE_PASS_BODY) {
        return source->draw_body != NULL;
    }
    if (pass == BAKED_SURFACE_PASS_OVERLAY) {
        return source->draw_overlay != NULL;
    }
    return false;
}

uint32_t resource_manager_get_bake_frame_count(
    const VisualSourceResource* source
) {
    uint32_t frame_count;

    if (source == NULL) {
        return 0U;
    }

    frame_count = source->bake_frame_count;
    if (frame_count > 0U && source->animation_fps > 0.0f && source->bake_animation_fps > 0.0f) {
        float loop_duration_seconds = (float)frame_count / source->animation_fps;
        frame_count = (uint32_t)ceilf(loop_duration_seconds * source->bake_animation_fps);
    }
    if (frame_count == 0U) {
        frame_count = source->bake_animation_fps > 0.0f
            ? (uint32_t)ceilf(source->bake_animation_fps)
            : (source->animation_fps > 0.0f ? (uint32_t)ceilf(source->animation_fps) : 1U);
    }
    if (frame_count == 0U) {
        frame_count = 1U;
    }

    return frame_count;
}
