#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCETYPES_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCETYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct ResourceHandle {
    uint32_t id;
} ResourceHandle;

typedef enum BakePolicy {
    BAKE_POLICY_DISABLED = 0,
    BAKE_POLICY_SHARED_FRAME,
    BAKE_POLICY_REFRESH_FRAME
} BakePolicy;

#define RESOURCE_DEFAULT_PROCEDURAL_BAKE_POLICY BAKE_POLICY_SHARED_FRAME

enum {
    RESOURCE_DEFAULT_PREBAKE_REQUIRED = 1,
    RESOURCE_DEFAULT_BAKE_INSTANCE_INVARIANT = 1
};

typedef enum BakedTexturePass {
    BAKED_TEXTURE_PASS_BODY = 0,
    BAKED_TEXTURE_PASS_OVERLAY = 1
} BakedTexturePass;

typedef struct GeneratedFrameConfig {
    float animation_fps;
    float cache_fps;
    bool loop;
    BakePolicy cache_policy;
    bool prebake_enabled;
    bool instance_invariant;
    uint32_t frame_count;
} GeneratedFrameConfig;

#endif
