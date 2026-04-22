#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_H

#include "SceneTypes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Entity;
struct PhysicsWorldConfig;

typedef struct Scene Scene;

typedef void (*SceneInputCallback)(Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data);

void Scene_Init(Scene* scene, const char* name, const struct PhysicsWorldConfig* physics_config);
Scene* Scene_Create(const char* name, const struct PhysicsWorldConfig* physics_config);
void Scene_Destroy(Scene* scene);

void Scene_SetInputCallback(Scene* scene, SceneInputCallback callback);

bool Scene_AddEntity(Scene* scene, struct Entity* entity);
struct Entity* Scene_RemoveEntity(Scene* scene, struct Entity* entity);
struct Entity* Scene_FindEntityById(Scene* scene, uint32_t entity_id);
const struct Entity* Scene_FindEntityByIdConst(const Scene* scene, uint32_t entity_id);

void Scene_Update(Scene* scene, float dt_seconds, const SceneInputState* input_state);
void Scene_FlushSpatialUpdates(Scene* scene);

#endif
