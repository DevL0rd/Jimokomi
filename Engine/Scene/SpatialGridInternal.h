#ifndef JIMOKOMI_ENGINE_SCENE_SPATIALGRID_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SPATIALGRID_INTERNAL_H

#include "SpatialGrid.h"

#include <stdint.h>

typedef struct SpatialGridEntry
{
    struct Entity* entity;
    Aabb bounds;
    SpatialGridCellSpan span;
    size_t node_start_index;
    size_t node_count;
    bool active;
    uint32_t last_query_stamp;
} SpatialGridEntry;

typedef struct SpatialGridNode
{
    size_t entry_index;
    int cell_index;
    int previous_index;
    int next_index;
} SpatialGridNode;

struct SpatialGrid
{
    Aabb bounds;
    float cell_size;
    size_t columns;
    size_t rows;
    size_t cell_count;
    int* cell_heads;
    uint32_t* cell_entry_counts;
    uint8_t* dirty_cell_flags;
    uint32_t* dirty_cell_indices;
    size_t dirty_cell_count;
    size_t dirty_cell_capacity;
    SpatialGridEntry* entries;
    size_t entry_count;
    size_t entry_capacity;
    SpatialGridNode* nodes;
    size_t node_count;
    size_t node_capacity;
    uint32_t query_stamp;
    bool enabled;
};

#endif
