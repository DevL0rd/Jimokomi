#include "PhysicsSyncSystem.h"

#include "../Scene.h"
#include "../../Physics/PhysicsWorld.h"

void PhysicsSyncSystem_Update(struct Scene* scene, float dt_seconds)
{
    if (scene == NULL || scene->physics_world == NULL || scene->physics_paused)
    {
        return;
    }

    PhysicsWorld_Update(scene->physics_world, scene, dt_seconds);
}
