#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_STATS_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_STATS_INTERNAL_H

#include "ResourceManagerInternal.h"

struct ResourceStatsState {
    uint64_t bake_cache_hits;
    uint64_t bake_cache_misses;
    uint64_t bake_invalidation_visual_source_count;
    uint64_t bake_invalidation_material_count;
    uint64_t bake_invalidation_shader_count;
    uint64_t bake_invalidation_animation_frame_count;
    uint64_t prebake_ready_visual_source_count;
    uint64_t prebake_ready_material_count;
};

#endif
