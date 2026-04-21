#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_PHYSICS_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_PHYSICS_H

#include "Scene.h"
#include "../Physics/PhysicsWorld.h"

void Scene_SetPhysicsPaused(Scene* scene, bool paused);
bool Scene_IsPhysicsPaused(const Scene* scene);
PhysicsWorld* Scene_GetPhysicsWorld(Scene* scene);
const PhysicsWorld* Scene_GetPhysicsWorldConst(const Scene* scene);
bool Scene_GetEntityLinearVelocity(Scene* scene, struct Entity* entity, Vec2* out_velocity);
bool Scene_SetEntityLinearVelocity(Scene* scene, struct Entity* entity, Vec2 velocity);
bool Scene_ApplyEntityLinearImpulse(Scene* scene, struct Entity* entity, Vec2 impulse, bool wake);
bool Scene_GetEntityContactCapacity(Scene* scene, struct Entity* entity, int* out_contact_capacity);
void Scene_SetTilemap(
    Scene* scene,
    const SceneTilemapAdapter* adapter,
    const void* tilemap,
    const TileRule* tile_rules,
    size_t tile_rule_count
);

#endif
