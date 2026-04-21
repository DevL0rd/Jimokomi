#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_QUERIES_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_QUERIES_H

#include "Scene.h"
#include "SpatialGrid.h"

struct Entity* Scene_PickEntityAtScreen(Scene* scene, float screen_x, float screen_y);
size_t Scene_QueryEntitiesInAabb(Scene* scene, Aabb bounds, struct Entity** results, size_t capacity);
size_t Scene_QueryEntitiesInRadius(Scene* scene, Vec2 center, float radius, struct Entity** results, size_t capacity);
size_t Scene_QueryEntitiesAlongSegment(Scene* scene, Vec2 start, Vec2 end, struct Entity** results, size_t capacity);
bool Scene_GetSpatialGridCellSpanForAabb(const Scene* scene, Aabb bounds, SpatialGridCellSpan* out_span);
size_t Scene_GetSpatialGridDirtyCellSpans(const Scene* scene, SpatialGridCellSpan* results, size_t capacity);
void Scene_GetSpatialGridStatsSnapshot(const Scene* scene, SpatialGridStatsSnapshot* out_snapshot);

#endif
