#include "ResourceManagerInternal.h"
#include "GeneratedFrame.h"

#include <math.h>

uint64_t resource_manager_hash_baked_key(BakedTextureKey key) {
    uint64_t hash = 1469598103934665603ULL;
    hash ^= (uint64_t)key.procedural_texture_id;
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

bool resource_manager_baked_key_equals(BakedTextureKey a, BakedTextureKey b) {
    return a.procedural_texture_id == b.procedural_texture_id &&
           a.material_id == b.material_id &&
           a.shader_id == b.shader_id &&
           a.frame_index == b.frame_index &&
           a.pass == b.pass;
}

uint32_t resource_manager_normalize_frame_index(
    const ProceduralTextureResource* source,
    uint32_t frame_index
) {
    if (source == NULL) {
        return frame_index;
    }

    return generated_frame_cache_index(&source->frames, frame_index);
}

bool resource_manager_is_bake_eligible(
    const ProceduralTextureResource* source,
    BakedTexturePass pass
) {
    if (source == NULL) {
        return false;
    }
    if (source->frames.cache_policy == BAKE_POLICY_DISABLED || !source->frames.instance_invariant) {
        return false;
    }
    if (pass == BAKED_TEXTURE_PASS_BODY) {
        return source->draw_body != NULL;
    }
    if (pass == BAKED_TEXTURE_PASS_OVERLAY) {
        return source->draw_overlay != NULL;
    }
    return false;
}

uint32_t resource_manager_get_bake_frame_count(
    const ProceduralTextureResource* source
) {
    if (source == NULL) {
        return 0U;
    }

    return generated_frame_cache_frame_count(&source->frames);
}
