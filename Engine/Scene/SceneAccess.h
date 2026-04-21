#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_ACCESS_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_ACCESS_H

#include "Scene.h"

void Scene_SetUserData(Scene* scene, void* user_data);
void* Scene_GetUserData(const Scene* scene);

#endif
