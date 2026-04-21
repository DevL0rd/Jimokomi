#include "SceneInternal.h"

#include "EntityInternal.h"
#include "SpatialGrid.h"
#include "Components/RandomForceComponent.h"
#include "Components/RigidBodyComponent.h"
#include "../Physics/PhysicsBodyControl.h"

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

    if (scene->storage.entity_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = scene->storage.entity_capacity == 0 ? 8 : scene->storage.entity_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    entities = (struct Entity**)realloc(scene->storage.entities, sizeof(struct Entity*) * new_capacity);
    if (entities == NULL)
    {
        return false;
    }

    scene->storage.entities = entities;
    scene->storage.entity_capacity = new_capacity;
    return true;
}

static bool Scene_ReserveDynamicEntitySlots(Scene* scene, size_t required_capacity)
{
    struct Entity** entities = NULL;
    size_t new_capacity = 0;

    if (scene == NULL)
    {
        return false;
    }

    if (scene->storage.dynamic_entity_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = scene->storage.dynamic_entity_capacity == 0 ? 8 : scene->storage.dynamic_entity_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    entities = (struct Entity**)realloc(scene->storage.dynamic_entities, sizeof(struct Entity*) * new_capacity);
    if (entities == NULL)
    {
        return false;
    }

    scene->storage.dynamic_entities = entities;
    scene->storage.dynamic_entity_capacity = new_capacity;
    return true;
}

static bool Scene_ReserveEntityListSlots(
    struct Entity*** entities,
    size_t* capacity,
    size_t required_capacity
)
{
    struct Entity** resized = NULL;
    size_t new_capacity = 0U;

    if (entities == NULL || capacity == NULL)
    {
        return false;
    }

    if (*capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = *capacity == 0U ? 8U : *capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2U;
    }

    resized = (struct Entity**)realloc(*entities, sizeof(struct Entity*) * new_capacity);
    if (resized == NULL)
    {
        return false;
    }

    *entities = resized;
    *capacity = new_capacity;
    return true;
}

bool Scene_AppendEntityToList(
    struct Entity*** entities,
    size_t* count,
    size_t* capacity,
    struct Entity* entity
)
{
    if (entities == NULL || count == NULL || capacity == NULL)
    {
        return false;
    }

    if (!Scene_ReserveEntityListSlots(entities, capacity, *count + 1U))
    {
        return false;
    }

    (*entities)[(*count)++] = entity;
    return true;
}

static void Scene_RemoveEntityFromList(struct Entity** entities, size_t* count, struct Entity* entity)
{
    size_t index = 0U;

    if (entities == NULL || count == NULL || entity == NULL)
    {
        return;
    }

    for (index = 0U; index < *count; ++index)
    {
        if (entities[index] == entity)
        {
            memmove(
                &entities[index],
                &entities[index + 1U],
                sizeof(struct Entity*) * (*count - index - 1U)
            );
            *count -= 1U;
            entities[*count] = NULL;
            return;
        }
    }
}

bool Scene_AddEntity(Scene* scene, struct Entity* entity)
{
    RigidBodyComponent* rigid_body = NULL;

    if (scene == NULL || entity == NULL)
    {
        return false;
    }

    if (entity->id == 0)
    {
        entity->id = scene->lifecycle.next_entity_id++;
    }
    else if (entity->id >= scene->lifecycle.next_entity_id)
    {
        scene->lifecycle.next_entity_id = entity->id + 1u;
    }

    if (!Scene_ReserveEntitySlots(scene, scene->storage.entity_count + 1))
    {
        return false;
    }

    entity->scene = scene;
    Entity_MarkDirty(
        entity,
        ENTITY_DIRTY_APPEARED | ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_LAYER_SORT
    );
    scene->storage.entities[scene->storage.entity_count++] = entity;
    if (scene->camera_follow.camera_target_entity == NULL &&
        Entity_GetComponent(entity, COMPONENT_CAMERA_TARGET) != NULL)
    {
        scene->camera_follow.camera_target_entity = entity;
    }
    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body != NULL && rigid_body->body_type != RIGID_BODY_STATIC)
    {
        if (!Scene_ReserveDynamicEntitySlots(scene, scene->storage.dynamic_entity_count + 1))
        {
            Scene_RemoveEntity(scene, entity);
            return false;
        }
        scene->storage.dynamic_entities[scene->storage.dynamic_entity_count++] = entity;
    }
    if (Entity_IsActive(entity) && Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM) != NULL &&
        !Scene_AppendEntityToList(&scene->storage.renderable_entities, &scene->storage.renderable_entity_count, &scene->storage.renderable_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Entity_GetComponent(entity, COMPONENT_RANDOM_FORCE) != NULL)
    {
        if (!Scene_AppendEntityToList(&scene->storage.random_force_entities, &scene->storage.random_force_entity_count, &scene->storage.random_force_entity_capacity, entity))
        {
            Scene_RemoveEntity(scene, entity);
            return false;
        }
    }
    if (scene->physics.physics_world != NULL)
    {
        PhysicsWorld_RegisterEntity(scene->physics.physics_world, entity);
    }
    SpatialGrid_UpdateEntity(&scene->spatial.spatial_grid, entity);
    return true;
}

struct Entity* Scene_RemoveEntity(Scene* scene, struct Entity* entity)
{
    size_t index = 0;

    if (scene == NULL || entity == NULL)
    {
        return NULL;
    }

    Scene_RemoveEntityFromList(scene->storage.dynamic_entities, &scene->storage.dynamic_entity_count, entity);
    Scene_RemoveEntityFromList(scene->storage.renderable_entities, &scene->storage.renderable_entity_count, entity);
    Scene_RemoveEntityFromList(scene->storage.random_force_entities, &scene->storage.random_force_entity_count, entity);
    SpatialGrid_RemoveEntity(&scene->spatial.spatial_grid, entity);
    if (scene->physics.physics_world != NULL)
    {
        PhysicsWorld_UnregisterEntity(scene->physics.physics_world, entity);
    }

    for (index = 0; index < scene->storage.entity_count; ++index)
    {
        if (scene->storage.entities[index] == entity)
        {
            PhysicsWorld_RemoveBodyForEntity(scene->physics.physics_world, entity);
            if (scene->camera_follow.camera_target_entity == entity)
            {
                scene->camera_follow.camera_target_entity = NULL;
            }
            Entity_MarkDirty(entity, ENTITY_DIRTY_REMOVED | ENTITY_DIRTY_VISIBILITY);
            memmove(&scene->storage.entities[index],
                    &scene->storage.entities[index + 1],
                    sizeof(struct Entity*) * (scene->storage.entity_count - index - 1));
            scene->storage.entity_count -= 1;
            scene->storage.entities[scene->storage.entity_count] = NULL;
            entity->scene = NULL;
            return entity;
        }
    }

    return NULL;
}

struct Entity* Scene_FindEntityById(Scene* scene, uint32_t entity_id)
{
    size_t index = 0U;

    if (scene == NULL || entity_id == 0U)
    {
        return NULL;
    }

    for (index = 0U; index < scene->storage.entity_count; ++index)
    {
        if (scene->storage.entities[index] != NULL && Entity_GetId(scene->storage.entities[index]) == entity_id)
        {
            return scene->storage.entities[index];
        }
    }

    return NULL;
}

const struct Entity* Scene_FindEntityByIdConst(const Scene* scene, uint32_t entity_id)
{
    return Scene_FindEntityById((Scene*)scene, entity_id);
}

void SceneStorage_Dispose(SceneStorage* storage)
{
    if (storage == NULL)
    {
        return;
    }

    free(storage->entities);
    storage->entities = NULL;
    storage->entity_count = 0U;
    storage->entity_capacity = 0U;

    free(storage->dynamic_entities);
    storage->dynamic_entities = NULL;
    storage->dynamic_entity_count = 0U;
    storage->dynamic_entity_capacity = 0U;

    free(storage->renderable_entities);
    storage->renderable_entities = NULL;
    storage->renderable_entity_count = 0U;
    storage->renderable_entity_capacity = 0U;

    free(storage->random_force_entities);
    storage->random_force_entities = NULL;
    storage->random_force_entity_count = 0U;
    storage->random_force_entity_capacity = 0U;
}
