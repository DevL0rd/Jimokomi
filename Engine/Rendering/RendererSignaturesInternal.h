#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERER_SIGNATURES_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERER_SIGNATURES_INTERNAL_H

#include "RendererInternal.h"

struct RendererSignatureState {
    uint32_t dirty_flags;
    uint64_t last_frame_signature;
    uint64_t last_sort_signature;
    uint64_t last_overlay_signature;
    uint64_t last_instance_batch_signature;
    uint64_t last_backdrop_signature;
    uint64_t last_metadata_signature;
};

#endif
