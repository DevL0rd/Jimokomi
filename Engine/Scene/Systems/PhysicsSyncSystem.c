#include "PhysicsSyncSystem.h"

#include "../SceneInternal.h"
#include "../../Physics/PhysicsWorld.h"

void PhysicsSyncSystem_Update(struct Scene* scene, float dt_seconds)
{
    if (scene == NULL || scene->physics_world == NULL || scene->physics_paused || dt_seconds <= 0.0f ||
        scene->dynamic_entity_count == 0U)
    {
        return;
    }

    PhysicsWorld_Update(scene->physics_world, scene, dt_seconds);
}
