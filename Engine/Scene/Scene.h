#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_H

#include "Component.h"
#include "SceneTypes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Entity;

typedef struct Scene
{
    char name[64];
    bool is_overlay;
    bool active;
    void* user_data;
    struct Entity** entities;
    size_t entity_count;
    size_t entity_capacity;
    uint32_t next_entity_id;
    PhysicsWorld* physics_world;
    bool physics_paused;
    const void* tilemap;
    const SceneTilemapAdapter* tilemap_adapter;
    const TileRule* tile_rules;
    size_t tile_rule_count;
    bool kill_plane_enabled;
    float kill_plane_margin;
    bool has_kill_bounds;
    Aabb kill_bounds;
    SceneView view;
    uint32_t last_physics_substeps;
    void (*on_input)(struct Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data);
} Scene;

void Scene_Init(Scene* scene, const char* name, const PhysicsWorldConfig* physics_config);
Scene* Scene_Create(const char* name, const PhysicsWorldConfig* physics_config);
void Scene_Destroy(Scene* scene);

void Scene_SetPhysicsPaused(Scene* scene, bool paused);
bool Scene_IsPhysicsPaused(const Scene* scene);
void Scene_UpdatePerformanceBudget(Scene* scene, float frame_ms);

bool Scene_AddEntity(Scene* scene, struct Entity* entity);
struct Entity* Scene_RemoveEntity(Scene* scene, struct Entity* entity);

void Scene_SetTilemap(Scene* scene,
                        const SceneTilemapAdapter* adapter,
                        const void* tilemap,
                        const TileRule* tile_rules,
                        size_t tile_rule_count);
void Scene_SetWorldBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y);
void Scene_SetKillBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y);
void Scene_ClearKillBounds(Scene* scene);
bool Scene_ShouldKillEntity(const Scene* scene, const struct Entity* entity);
void Scene_ApplyKillPlane(Scene* scene);

void Scene_Update(Scene* scene, float dt_seconds, const SceneInputState* input_state);
struct Entity* Scene_PickEntityAtScreen(Scene* scene, float screen_x, float screen_y);

#endif
