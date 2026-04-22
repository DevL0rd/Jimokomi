#include "PhysicsSyncSystem.h"

#include "../SceneInternal.h"
#include "../ScenePhysicsInternal.h"
#include "../SceneStorageInternal.h"
#include "../../Physics/PhysicsParticles.h"
#include "../../Physics/PhysicsWorld.h"

void PhysicsSyncSystem_Update(struct Scene* scene, float dt_seconds)
{
    PhysicsWorld* physics_world = NULL;

    if (scene == NULL || scene->physics->physics_world == NULL || scene->physics->physics_paused || dt_seconds <= 0.0f)
    {
        return;
    }

    physics_world = scene->physics->physics_world;
    if (scene->storage->dynamic_entity_count == 0U && PhysicsWorld_GetParticleCount(physics_world) == 0U)
    {
        return;
    }

    PhysicsWorld_Update(physics_world, scene, dt_seconds);
}
