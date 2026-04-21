#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_H

#include "Component.h"
#include "SceneTypes.h"
#include "SpatialGrid.h"
#include "../Physics/PhysicsWorld.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Entity;

typedef struct Scene Scene;

typedef struct SceneStatsSnapshot {
    size_t entity_count;
    uint32_t spatial_dirty_cell_count;
    uint32_t spatial_dirty_entity_count;
    double input_route_ms;
    double random_force_ms;
    double physics_sync_ms;
    double spatial_grid_ms;
    double camera_follow_ms;
    uint32_t physics_substeps;
} SceneStatsSnapshot;

typedef void (*SceneInputCallback)(Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data);

void Scene_Init(Scene* scene, const char* name, const PhysicsWorldConfig* physics_config);
Scene* Scene_Create(const char* name, const PhysicsWorldConfig* physics_config);
void Scene_Destroy(Scene* scene);

void Scene_SetPhysicsPaused(Scene* scene, bool paused);
bool Scene_IsPhysicsPaused(const Scene* scene);
void Scene_SetUserData(Scene* scene, void* user_data);
void* Scene_GetUserData(const Scene* scene);
void Scene_SetInputCallback(Scene* scene, SceneInputCallback callback);
PhysicsWorld* Scene_GetPhysicsWorld(Scene* scene);
const PhysicsWorld* Scene_GetPhysicsWorldConst(const Scene* scene);
SceneView Scene_GetViewSnapshot(const Scene* scene);
void Scene_SetView(Scene* scene, const SceneView* view);
size_t Scene_GetEntityCount(const Scene* scene);
void Scene_GetStatsSnapshot(const Scene* scene, SceneStatsSnapshot* out_snapshot);
void Scene_GetSpatialGridStatsSnapshot(const Scene* scene, SpatialGridStatsSnapshot* out_snapshot);

bool Scene_AddEntity(Scene* scene, struct Entity* entity);
struct Entity* Scene_RemoveEntity(Scene* scene, struct Entity* entity);

void Scene_SetTilemap(Scene* scene,
                        const SceneTilemapAdapter* adapter,
                        const void* tilemap,
                        const TileRule* tile_rules,
                        size_t tile_rule_count);
void Scene_SetWorldBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y);
bool Scene_GetSpatialGridCellSpanForAabb(const Scene* scene, Aabb bounds, SpatialGridCellSpan* out_span);
size_t Scene_GetSpatialGridDirtyCellSpans(const Scene* scene, SpatialGridCellSpan* results, size_t capacity);

void Scene_Update(Scene* scene, float dt_seconds, const SceneInputState* input_state);
struct Entity* Scene_PickEntityAtScreen(Scene* scene, float screen_x, float screen_y);
size_t Scene_QueryEntitiesInAabb(Scene* scene, Aabb bounds, struct Entity** results, size_t capacity);
size_t Scene_QueryEntitiesInRadius(Scene* scene, Vec2 center, float radius, struct Entity** results, size_t capacity);
size_t Scene_QueryEntitiesAlongSegment(Scene* scene, Vec2 start, Vec2 end, struct Entity** results, size_t capacity);

#endif
