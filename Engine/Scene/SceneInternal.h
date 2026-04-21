#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_INTERNAL_H

#include "Scene.h"

void Scene_UpdateSpatialGridBounds(Scene* scene);

typedef struct SceneStorage SceneStorage;
typedef struct SceneLifecycleState SceneLifecycleState;
typedef struct ScenePhysicsState ScenePhysicsState;
typedef struct SceneTilemapState SceneTilemapState;
typedef struct SceneSpatialState SceneSpatialState;
typedef struct SceneStatsState SceneStatsState;
typedef struct SceneCameraFollowState SceneCameraFollowState;

typedef struct Scene
{
    SceneLifecycleState* lifecycle;
    SceneStorage* storage;
    ScenePhysicsState* physics;
    SceneTilemapState* tilemap;
    SceneSpatialState* spatial;
    SceneStatsState* stats;
    SceneCameraFollowState* camera_follow;
} Scene;

#endif
