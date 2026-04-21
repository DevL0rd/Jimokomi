#ifndef JIMOKOMI_ENGINE_SCENE_SPATIALGRID_H
#define JIMOKOMI_ENGINE_SCENE_SPATIALGRID_H

#include "../Core/Geometry.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Entity;

typedef struct SpatialGridCellSpan
{
    int min_cell_x;
    int max_cell_x;
    int min_cell_y;
    int max_cell_y;
    uint32_t cell_count;
    bool valid;
} SpatialGridCellSpan;

typedef struct SpatialGrid SpatialGrid;

typedef struct SpatialGridStatsSnapshot {
    bool enabled;
    Aabb bounds;
    float cell_size;
} SpatialGridStatsSnapshot;

void SpatialGrid_Init(SpatialGrid* grid, float cell_size);
void SpatialGrid_Dispose(SpatialGrid* grid);
void SpatialGrid_GetStatsSnapshot(
    const SpatialGrid* grid,
    SpatialGridStatsSnapshot* out_snapshot
);
void SpatialGrid_SetBounds(SpatialGrid* grid, Aabb bounds);
void SpatialGrid_ClearBounds(SpatialGrid* grid);
bool SpatialGrid_GetCellSpanForAabb(const SpatialGrid* grid, Aabb bounds, SpatialGridCellSpan* out_span);
void SpatialGrid_Rebuild(SpatialGrid* grid, struct Entity* const* entities, size_t entity_count);
bool SpatialGrid_UpdateEntity(SpatialGrid* grid, struct Entity* entity);
void SpatialGrid_RemoveEntity(SpatialGrid* grid, struct Entity* entity);
size_t SpatialGrid_GetDirtyCellSpans(const SpatialGrid* grid, SpatialGridCellSpan* results, size_t capacity);
void SpatialGrid_ClearDirtyCells(SpatialGrid* grid);
size_t SpatialGrid_QueryAabb(SpatialGrid* grid, Aabb bounds, struct Entity** results, size_t capacity);
size_t SpatialGrid_QueryCircle(SpatialGrid* grid, Vec2 center, float radius, struct Entity** results, size_t capacity);
size_t SpatialGrid_QuerySegment(SpatialGrid* grid, Vec2 start, Vec2 end, struct Entity** results, size_t capacity);

#endif
