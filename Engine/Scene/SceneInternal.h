#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_INTERNAL_H

#include "Scene.h"
#include "SpatialGridInternal.h"

typedef struct Scene
{
    char name[64];
    bool is_overlay;
    bool active;
    void* user_data;
    struct Entity** entities;
    struct Entity* camera_target_entity;
    struct Entity** dynamic_entities;
    struct Entity** selectable_entities;
    struct Entity** draggable_entities;
    struct Entity** renderable_entities;
    struct Entity** debug_visible_entities;
    struct Entity** trigger_query_entities;
    struct Entity** proximity_query_entities;
    struct Entity** random_force_entities;
    size_t entity_count;
    size_t entity_capacity;
    size_t dynamic_entity_count;
    size_t dynamic_entity_capacity;
    size_t selectable_entity_count;
    size_t selectable_entity_capacity;
    size_t draggable_entity_count;
    size_t draggable_entity_capacity;
    size_t renderable_entity_count;
    size_t renderable_entity_capacity;
    size_t debug_visible_entity_count;
    size_t debug_visible_entity_capacity;
    size_t trigger_query_entity_count;
    size_t trigger_query_entity_capacity;
    size_t proximity_query_entity_count;
    size_t proximity_query_entity_capacity;
    size_t random_force_entity_capacity;
    size_t random_force_entity_count;
    uint32_t next_entity_id;
    PhysicsWorld* physics_world;
    bool physics_paused;
    const void* tilemap;
    const SceneTilemapAdapter* tilemap_adapter;
    const TileRule* tile_rules;
    size_t tile_rule_count;
    SceneView view;
    SpatialGrid spatial_grid;
    double last_input_route_ms;
    double last_random_force_ms;
    double last_physics_sync_ms;
    double last_camera_follow_ms;
    uint32_t last_physics_substeps;
    struct Entity* last_camera_follow_entity;
    float last_camera_follow_target_x;
    float last_camera_follow_target_y;
    float last_camera_follow_view_width;
    float last_camera_follow_view_height;
    void (*on_input)(struct Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data);
} Scene;

#endif
