#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTINTERNAL_H

#include "RenderSnapshot.h"

bool render_world_snapshot_reserve_items(RenderWorldSnapshot* snapshot, size_t required_capacity);
bool render_world_snapshot_reserve_debug_collisions(RenderWorldSnapshot* snapshot, size_t required_capacity);
void render_world_snapshot_reset(RenderWorldSnapshot* snapshot);

#endif
