#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_CACHE_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_CACHE_INTERNAL_H

#include "ResourceManagerInternal.h"

typedef struct BakedSurfaceResource {
    BakedSurfaceKey key;
    Surface* surface;
} BakedSurfaceResource;

typedef struct BakedSurfaceSlot {
    uint8_t state;
    BakedSurfaceKey key;
    size_t index;
} BakedSurfaceSlot;

struct ResourceBakeCacheState {
    BakedSurfaceResource* baked_surfaces;
    size_t baked_surface_count;
    size_t baked_surface_capacity;
    BakedSurfaceSlot* baked_surface_slots;
    size_t baked_surface_slot_capacity;
    size_t baked_surface_slot_count;
};

bool resource_manager_reserve_baked_surfaces(ResourceManager* manager, size_t required);
bool resource_manager_insert_baked_surface_slot(ResourceManager* manager, BakedSurfaceKey key, size_t value_index);

#endif
