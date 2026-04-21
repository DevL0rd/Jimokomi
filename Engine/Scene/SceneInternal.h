#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_INTERNAL_H

#include "Scene.h"
#include "SceneStats.h"
#include "SpatialGridInternal.h"
#include "../Physics/PhysicsWorld.h"

void Scene_UpdateSpatialGridBounds(Scene* scene);

typedef struct SceneStorage
{
    struct Entity** entities;
    struct Entity** dynamic_entities;
    struct Entity** renderable_entities;
    struct Entity** random_force_entities;
    size_t entity_count;
    size_t entity_capacity;
    size_t dynamic_entity_count;
    size_t dynamic_entity_capacity;
    size_t renderable_entity_count;
    size_t renderable_entity_capacity;
    size_t random_force_entity_capacity;
    size_t random_force_entity_count;
} SceneStorage;

typedef struct SceneLifecycleState {
    char name[64];
    bool is_overlay;
    bool active;
    void* user_data;
    uint32_t next_entity_id;
    void (*on_input)(struct Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data);
} SceneLifecycleState;

typedef struct ScenePhysicsState {
    PhysicsWorld* physics_world;
    bool physics_paused;
} ScenePhysicsState;

typedef struct SceneTilemapState {
    const void* tilemap;
    const SceneTilemapAdapter* tilemap_adapter;
    const TileRule* tile_rules;
    size_t tile_rule_count;
} SceneTilemapState;

typedef struct SceneSpatialState {
    SceneView view;
    SpatialGrid spatial_grid;
} SceneSpatialState;

typedef struct SceneStatsState {
    double last_input_route_ms;
    double last_random_force_ms;
    double last_physics_sync_ms;
    double last_camera_follow_ms;
    uint32_t last_physics_substeps;
} SceneStatsState;

typedef struct SceneCameraFollowState {
    struct Entity* camera_target_entity;
    struct Entity* last_camera_follow_entity;
    float last_camera_follow_target_x;
    float last_camera_follow_target_y;
    float last_camera_follow_view_width;
    float last_camera_follow_view_height;
} SceneCameraFollowState;

typedef struct Scene
{
    SceneLifecycleState lifecycle;
    SceneStorage storage;
    ScenePhysicsState physics;
    SceneTilemapState tilemap;
    SceneSpatialState spatial;
    SceneStatsState stats;
    SceneCameraFollowState camera_follow;
} Scene;

#endif
