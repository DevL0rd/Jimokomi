#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCETYPES_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCETYPES_H

#include <stdint.h>

typedef struct ResourceHandle {
    uint32_t id;
} ResourceHandle;

typedef enum BakePolicy {
    BAKE_POLICY_DISABLED = 0,
    BAKE_POLICY_SHARED_FRAME
} BakePolicy;

typedef enum BakedSurfacePass {
    BAKED_SURFACE_PASS_BODY = 0,
    BAKED_SURFACE_PASS_OVERLAY = 1
} BakedSurfacePass;

#endif
