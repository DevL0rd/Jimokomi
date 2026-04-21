#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_VIEW_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_VIEW_H

#include "Scene.h"

SceneView Scene_GetViewSnapshot(const Scene* scene);
void Scene_SetView(Scene* scene, const SceneView* view);
void Scene_SetWorldBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y);

#endif
