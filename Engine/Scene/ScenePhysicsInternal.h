#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_PHYSICS_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_PHYSICS_INTERNAL_H

#include "SceneInternal.h"
#include "../Physics/PhysicsWorld.h"

struct ScenePhysicsState {
    PhysicsWorld* physics_world;
    bool physics_paused;
};

#endif
