#include "Scene.h"

#include "Entity.h"
#include "Systems/CameraFollowSystem.h"
#include "Systems/InputRoutingSystem.h"
#include "Systems/PhysicsSyncSystem.h"
#include "Components/ColliderComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/TransformComponent.h"

#include <stdlib.h>
#include <string.h>

static bool Scene_ReserveEntitySlots(Scene* scene, size_t required_capacity)
{
    struct Entity** entities = NULL;
    size_t new_capacity = 0;

    if (scene == NULL)
    {
        return false;
    }

    if (scene->entity_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = scene->entity_capacity == 0 ? 8 : scene->entity_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    entities = (struct Entity**)realloc(scene->entities, sizeof(struct Entity*) * new_capacity);
    if (entities == NULL)
    {
        return false;
    }

    scene->entities = entities;
    scene->entity_capacity = new_capacity;
    return true;
}

static void Scene_UpdateBoundsFromTilemap(Scene* scene)
{
    int pixel_width = 0;
    int pixel_height = 0;

    if (scene == NULL || scene->tilemap_adapter == NULL || scene->tilemap == NULL || scene->is_overlay)
    {
        Scene_ClearKillBounds(scene);
        scene->view.has_world_bounds = false;
        return;
    }

    if (scene->tilemap_adapter->get_pixel_width == NULL || scene->tilemap_adapter->get_pixel_height == NULL)
    {
        return;
    }

    pixel_width = scene->tilemap_adapter->get_pixel_width(scene->tilemap);
    pixel_height = scene->tilemap_adapter->get_pixel_height(scene->tilemap);
    if (pixel_width > 0 && pixel_height > 0)
    {
        Scene_SetWorldBounds(scene, 0.0f, 0.0f, (float)pixel_width, (float)pixel_height);
        Scene_SetKillBounds(scene, 0.0f, 0.0f, (float)pixel_width, (float)pixel_height);
    }
}

void Scene_Init(Scene* scene, const char* name, const PhysicsWorldConfig* physics_config)
{
    if (scene == NULL)
    {
        return;
    }

    memset(scene, 0, sizeof(*scene));
    strncpy(scene->name, name != NULL ? name : "Scene", sizeof(scene->name) - 1);
    scene->active = true;
    scene->next_entity_id = 1u;
    scene->physics_world = PhysicsWorld_Create(physics_config);
    scene->kill_plane_enabled = true;
    scene->kill_plane_margin = 64.0f;
    scene->view = (SceneView){0.0f, 0.0f, 480.0f, 270.0f, 0.12f, false, 0.0f, 0.0f, 0.0f, 0.0f};
}

Scene* Scene_Create(const char* name, const PhysicsWorldConfig* physics_config)
{
    Scene* scene = (Scene*)calloc(1, sizeof(Scene));
    if (scene == NULL)
    {
        return NULL;
    }

    Scene_Init(scene, name, physics_config);
    return scene;
}

void Scene_SetPhysicsPaused(Scene* scene, bool paused)
{
    if (scene != NULL)
    {
        scene->physics_paused = paused;
    }
}

bool Scene_IsPhysicsPaused(const Scene* scene)
{
    return scene != NULL && scene->physics_paused;
}

void Scene_UpdatePerformanceBudget(Scene* scene, float frame_ms)
{
    if (scene != NULL && scene->physics_world != NULL)
    {
        PhysicsWorld_UpdateAdaptiveBudget(scene->physics_world, frame_ms);
    }
}

bool Scene_AddEntity(Scene* scene, struct Entity* entity)
{
    if (scene == NULL || entity == NULL)
    {
        return false;
    }

    if (entity->id == 0)
    {
        entity->id = scene->next_entity_id++;
    }
    else if (entity->id >= scene->next_entity_id)
    {
        scene->next_entity_id = entity->id + 1u;
    }

    if (!Scene_ReserveEntitySlots(scene, scene->entity_count + 1))
    {
        return false;
    }

    entity->scene = scene;
    scene->entities[scene->entity_count++] = entity;
    return true;
}

struct Entity* Scene_RemoveEntity(Scene* scene, struct Entity* entity)
{
    size_t index = 0;

    if (scene == NULL || entity == NULL)
    {
        return NULL;
    }

    for (index = 0; index < scene->entity_count; ++index)
    {
        if (scene->entities[index] == entity)
        {
            PhysicsWorld_RemoveBodyForEntity(scene->physics_world, entity);
            memmove(&scene->entities[index],
                    &scene->entities[index + 1],
                    sizeof(struct Entity*) * (scene->entity_count - index - 1));
            scene->entity_count -= 1;
            scene->entities[scene->entity_count] = NULL;
            entity->scene = NULL;
            return entity;
        }
    }

    return NULL;
}

void Scene_SetTilemap(Scene* scene,
                        const SceneTilemapAdapter* adapter,
                        const void* tilemap,
                        const TileRule* tile_rules,
                        size_t tile_rule_count)
{
    if (scene == NULL)
    {
        return;
    }

    scene->tilemap_adapter = adapter;
    scene->tilemap = tilemap;
    scene->tile_rules = tile_rules;
    scene->tile_rule_count = tile_rule_count;
    if (scene->physics_world != NULL)
    {
        PhysicsWorld_SetTilemap(scene->physics_world, adapter, tilemap, tile_rules, tile_rule_count);
    }
    Scene_UpdateBoundsFromTilemap(scene);
}

void Scene_SetWorldBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y)
{
    if (scene == NULL)
    {
        return;
    }

    scene->view.has_world_bounds = true;
    scene->view.world_min_x = min_x;
    scene->view.world_min_y = min_y;
    scene->view.world_max_x = max_x;
    scene->view.world_max_y = max_y;
}

void Scene_SetKillBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y)
{
    if (scene == NULL)
    {
        return;
    }

    scene->kill_bounds = (Aabb){min_x, min_y, max_x, max_y};
    scene->has_kill_bounds = true;
}

void Scene_ClearKillBounds(Scene* scene)
{
    if (scene != NULL)
    {
        scene->has_kill_bounds = false;
    }
}

bool Scene_ShouldKillEntity(const Scene* scene, const struct Entity* entity)
{
    const TransformComponent* transform = NULL;
    const RigidBodyComponent* rigid_body = NULL;
    const ColliderComponent* collider = NULL;
    float half_w = 0.0f;
    float half_h = 0.0f;
    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;

    if (scene == NULL || entity == NULL || !scene->kill_plane_enabled || !scene->has_kill_bounds || !entity->active)
    {
        return false;
    }

    transform = (const TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
    rigid_body = (const RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    collider = (const ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
    if (transform == NULL || rigid_body == NULL)
    {
        return false;
    }
    if (rigid_body->body_type == RIGID_BODY_STATIC)
    {
        return false;
    }

    if (collider != NULL)
    {
        if (collider->shape == COLLIDER_SHAPE_RECT)
        {
            half_w = collider->width * 0.5f;
            half_h = collider->height * 0.5f;
        }
        else
        {
            half_w = collider->radius;
            half_h = collider->radius;
        }
    }

    min_x = scene->kill_bounds.min_x - scene->kill_plane_margin - half_w;
    min_y = scene->kill_bounds.min_y - scene->kill_plane_margin - half_h;
    max_x = scene->kill_bounds.max_x + scene->kill_plane_margin + half_w;
    max_y = scene->kill_bounds.max_y + scene->kill_plane_margin + half_h;
    return transform->x < min_x || transform->x > max_x || transform->y < min_y || transform->y > max_y;
}

void Scene_ApplyKillPlane(Scene* scene)
{
    size_t index = 0;

    if (scene == NULL || !scene->kill_plane_enabled || !scene->has_kill_bounds)
    {
        return;
    }

    index = scene->entity_count;
    while (index > 0)
    {
        struct Entity* entity = scene->entities[index - 1];
        if (entity != NULL && Scene_ShouldKillEntity(scene, entity))
        {
            Scene_RemoveEntity(scene, entity);
            Entity_Destroy(entity);
        }
        --index;
    }
}

void Scene_Update(Scene* scene, float dt_seconds, const SceneInputState* input_state)
{
    if (scene == NULL)
    {
        return;
    }

    InputRoutingSystem_Update(scene, input_state, dt_seconds);
    PhysicsSyncSystem_Update(scene, dt_seconds);
    CameraFollowSystem_Update(scene, dt_seconds);
    Scene_ApplyKillPlane(scene);
}

struct Entity* Scene_PickEntityAtScreen(Scene* scene, float screen_x, float screen_y)
{
    PhysicsQueryResult hits[16];
    size_t hit_count = 0;
    size_t index = 0;
    float world_x = 0.0f;
    float world_y = 0.0f;

    if (scene == NULL || scene->is_overlay)
    {
        return NULL;
    }

    world_x = scene->view.x + screen_x;
    world_y = scene->view.y + screen_y;
    hit_count = PhysicsWorld_QueryPoint(scene->physics_world, world_x, world_y, hits, 16);
    while (hit_count > 0)
    {
        if (hits[hit_count - 1].entity != NULL && hits[hit_count - 1].entity->active)
        {
            return hits[hit_count - 1].entity;
        }
        --hit_count;
    }

    for (index = scene->entity_count; index > 0; --index)
    {
        struct Entity* entity = scene->entities[index - 1];
        TransformComponent* transform = NULL;
        ColliderComponent* collider = NULL;

        if (entity == NULL || !entity->active)
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
        collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
        if (transform == NULL || collider == NULL)
        {
            continue;
        }

        if (collider->shape == COLLIDER_SHAPE_CIRCLE)
        {
            float dx = world_x - transform->x;
            float dy = world_y - transform->y;
            if ((dx * dx) + (dy * dy) <= collider->radius * collider->radius)
            {
                return entity;
            }
        }
        else
        {
            float half_w = collider->width * 0.5f;
            float half_h = collider->height * 0.5f;
            if (world_x >= transform->x - half_w && world_x <= transform->x + half_w && world_y >= transform->y - half_h &&
                world_y <= transform->y + half_h)
            {
                return entity;
            }
        }
    }

    return NULL;
}

void Scene_Destroy(Scene* scene)
{
    size_t index = 0;

    if (scene == NULL)
    {
        return;
    }

    for (index = scene->entity_count; index > 0; --index)
    {
        struct Entity* entity = scene->entities[index - 1];
        if (entity != NULL)
        {
            PhysicsWorld_RemoveBodyForEntity(scene->physics_world, entity);
            entity->scene = NULL;
            Entity_Destroy(entity);
        }
    }

    free(scene->entities);
    scene->entities = NULL;
    scene->entity_count = 0;
    scene->entity_capacity = 0;

    if (scene->physics_world != NULL)
    {
        PhysicsWorld_Destroy(scene->physics_world);
        scene->physics_world = NULL;
    }

    free(scene);
}
