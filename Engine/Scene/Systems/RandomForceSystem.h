#ifndef JIMOKOMI_ENGINE_SCENE_SYSTEMS_RANDOMFORCESYSTEM_H
#define JIMOKOMI_ENGINE_SCENE_SYSTEMS_RANDOMFORCESYSTEM_H

#include <stdbool.h>

struct Entity;
struct Scene;

bool RandomForceSystem_AddToEntity(
    struct Scene* scene,
    struct Entity* entity,
    float force_strength,
    float interval_seconds
);
void RandomForceSystem_Update(struct Scene* scene, float dt_seconds);

#endif
