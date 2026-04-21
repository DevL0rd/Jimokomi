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
}
