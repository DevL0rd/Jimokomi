#include "SpatialGridInternal.h"

#include <string.h>

void SpatialGrid_GetStatsSnapshot(
    const SpatialGrid* grid,
    SpatialGridStatsSnapshot* out_snapshot
) {
    if (out_snapshot == NULL)
    {
        return;
    }
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    if (grid == NULL)
    {
        return;
    }

    out_snapshot->enabled = grid->enabled;
    out_snapshot->bounds = grid->bounds;
    out_snapshot->cell_size = grid->cell_size;
    out_snapshot->columns = grid->columns;
    out_snapshot->rows = grid->rows;
    out_snapshot->cell_count = grid->cell_count;
    out_snapshot->entry_count = grid->entry_count;
    out_snapshot->entry_capacity = grid->entry_capacity;
    out_snapshot->node_count = grid->node_count;
    out_snapshot->node_capacity = grid->node_capacity;
    out_snapshot->dirty_cell_capacity = grid->dirty_cell_capacity;
    out_snapshot->rebuild_ms = grid->last_rebuild_ms;
    out_snapshot->incremental_update_ms = grid->last_incremental_update_ms;
    out_snapshot->query_ms = grid->last_query_ms;
    out_snapshot->query_cells = grid->last_query_cells;
    out_snapshot->query_hits = grid->last_query_hits;
    out_snapshot->query_false_positives = grid->last_query_false_positives;
    out_snapshot->rebuild_non_empty_cells = grid->last_rebuild_non_empty_cells;
    out_snapshot->rebuild_max_cell_entries = grid->last_rebuild_max_cell_entries;
    out_snapshot->rebuild_hottest_cell_ms = grid->last_rebuild_hottest_cell_ms;
    out_snapshot->query_max_cell_visits = grid->last_query_max_cell_visits;
    out_snapshot->query_hottest_cell_ms = grid->last_query_hottest_cell_ms;
    out_snapshot->dirty_cell_count = grid->last_dirty_cell_count;
    out_snapshot->incremental_dirty_entity_count = grid->last_incremental_dirty_entity_count;
    out_snapshot->full_rebuild_count = grid->full_rebuild_count;
    out_snapshot->incremental_update_count = grid->incremental_update_count;
    out_snapshot->memory_bytes =
        grid->columns * grid->rows * (sizeof(int) + sizeof(uint32_t) + sizeof(uint8_t)) +
        grid->entry_capacity * sizeof(SpatialGridEntry) +
        grid->node_capacity * sizeof(SpatialGridNode) +
        grid->dirty_cell_capacity * sizeof(uint32_t);
}
