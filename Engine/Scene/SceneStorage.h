#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_STORAGE_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_STORAGE_H

#include "SceneInternal.h"

bool Scene_AppendEntityToList(
    struct Entity*** entities,
    size_t* count,
    size_t* capacity,
    struct Entity* entity
);

void SceneStorage_Dispose(SceneStorage* storage);

#endif
