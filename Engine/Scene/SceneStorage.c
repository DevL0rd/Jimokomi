#include "SceneInternal.h"

#include "EntityInternal.h"
#include "SpatialGrid.h"
#include "Components/ColliderComponent.h"
#include "Components/DraggableComponent.h"
#include "Components/RandomForceComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SelectableComponent.h"
#include "Components/TransformComponent.h"
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

static bool Scene_ReserveDynamicEntitySlots(Scene* scene, size_t required_capacity)
{
    struct Entity** entities = NULL;
    size_t new_capacity = 0;

    if (scene == NULL)
    {
        return false;
    }

    if (scene->dynamic_entity_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = scene->dynamic_entity_capacity == 0 ? 8 : scene->dynamic_entity_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    entities = (struct Entity**)realloc(scene->dynamic_entities, sizeof(struct Entity*) * new_capacity);
    if (entities == NULL)
    {
        return false;
    }

    scene->dynamic_entities = entities;
    scene->dynamic_entity_capacity = new_capacity;
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

static bool Scene_IsSelectableEntity(const Entity* entity)
{
    return entity != NULL &&
           Entity_IsActive(entity) &&
           Entity_GetFixedComponent(entity, COMPONENT_SELECTABLE) != NULL &&
           Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM) != NULL &&
           Entity_GetFixedComponent(entity, COMPONENT_COLLIDER) != NULL;
}

static bool Scene_IsDraggableEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity) &&
           Entity_GetFixedComponent(entity, COMPONENT_DRAGGABLE) != NULL;
}

static bool Scene_IsRenderableEntity(const Entity* entity)
{
    return entity != NULL &&
           Entity_IsActive(entity) &&
           Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM) != NULL;
}

static bool Scene_IsDebugVisibleEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity);
}

static bool Scene_IsTriggerQueryEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity);
}

static bool Scene_IsProximityQueryEntity(const Entity* entity)
{
    return Scene_IsSelectableEntity(entity);
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
    Entity_MarkDirty(
        entity,
        ENTITY_DIRTY_APPEARED | ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_LAYER_SORT
    );
    scene->entities[scene->entity_count++] = entity;
    if (scene->camera_target_entity == NULL &&
        Entity_GetComponent(entity, COMPONENT_CAMERA_TARGET) != NULL)
    {
        scene->camera_target_entity = entity;
    }
    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body != NULL && rigid_body->body_type != RIGID_BODY_STATIC)
    {
        if (!Scene_ReserveDynamicEntitySlots(scene, scene->dynamic_entity_count + 1))
        {
            Scene_RemoveEntity(scene, entity);
            return false;
        }
        scene->dynamic_entities[scene->dynamic_entity_count++] = entity;
    }
    if (Scene_IsSelectableEntity(entity) &&
        !Scene_AppendEntityToList(&scene->selectable_entities, &scene->selectable_entity_count, &scene->selectable_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsDraggableEntity(entity) &&
        !Scene_AppendEntityToList(&scene->draggable_entities, &scene->draggable_entity_count, &scene->draggable_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsRenderableEntity(entity) &&
        !Scene_AppendEntityToList(&scene->renderable_entities, &scene->renderable_entity_count, &scene->renderable_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsDebugVisibleEntity(entity) &&
        !Scene_AppendEntityToList(&scene->debug_visible_entities, &scene->debug_visible_entity_count, &scene->debug_visible_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsTriggerQueryEntity(entity) &&
        !Scene_AppendEntityToList(&scene->trigger_query_entities, &scene->trigger_query_entity_count, &scene->trigger_query_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Scene_IsProximityQueryEntity(entity) &&
        !Scene_AppendEntityToList(&scene->proximity_query_entities, &scene->proximity_query_entity_count, &scene->proximity_query_entity_capacity, entity))
    {
        Scene_RemoveEntity(scene, entity);
        return false;
    }
    if (Entity_GetComponent(entity, COMPONENT_RANDOM_FORCE) != NULL)
    {
        if (!Scene_AppendEntityToList(&scene->random_force_entities, &scene->random_force_entity_count, &scene->random_force_entity_capacity, entity))
        {
            Scene_RemoveEntity(scene, entity);
            return false;
        }
    }
    if (scene->physics_world != NULL)
    {
        PhysicsWorld_RegisterEntity(scene->physics_world, entity);
    }
    SpatialGrid_UpdateEntity(&scene->spatial_grid, entity);
    return true;
}

struct Entity* Scene_RemoveEntity(Scene* scene, struct Entity* entity)
{
    size_t index = 0;

    if (scene == NULL || entity == NULL)
    {
        return NULL;
    }

    Scene_RemoveEntityFromList(scene->dynamic_entities, &scene->dynamic_entity_count, entity);
    Scene_RemoveEntityFromList(scene->selectable_entities, &scene->selectable_entity_count, entity);
    Scene_RemoveEntityFromList(scene->draggable_entities, &scene->draggable_entity_count, entity);
    Scene_RemoveEntityFromList(scene->renderable_entities, &scene->renderable_entity_count, entity);
    Scene_RemoveEntityFromList(scene->debug_visible_entities, &scene->debug_visible_entity_count, entity);
    Scene_RemoveEntityFromList(scene->trigger_query_entities, &scene->trigger_query_entity_count, entity);
    Scene_RemoveEntityFromList(scene->proximity_query_entities, &scene->proximity_query_entity_count, entity);
    Scene_RemoveEntityFromList(scene->random_force_entities, &scene->random_force_entity_count, entity);
    SpatialGrid_RemoveEntity(&scene->spatial_grid, entity);
    if (scene->physics_world != NULL)
    {
        PhysicsWorld_UnregisterEntity(scene->physics_world, entity);
    }

    for (index = 0; index < scene->entity_count; ++index)
    {
        if (scene->entities[index] == entity)
        {
            PhysicsWorld_RemoveBodyForEntity(scene->physics_world, entity);
            if (scene->camera_target_entity == entity)
            {
                scene->camera_target_entity = NULL;
            }
            Entity_MarkDirty(entity, ENTITY_DIRTY_REMOVED | ENTITY_DIRTY_VISIBILITY);
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

struct Entity* Scene_FindEntityById(Scene* scene, uint32_t entity_id)
{
    size_t index = 0U;

    if (scene == NULL || entity_id == 0U)
    {
        return NULL;
    }

    for (index = 0U; index < scene->entity_count; ++index)
    {
        if (scene->entities[index] != NULL && Entity_GetId(scene->entities[index]) == entity_id)
        {
            return scene->entities[index];
        }
    }

    return NULL;
}

const struct Entity* Scene_FindEntityByIdConst(const Scene* scene, uint32_t entity_id)
{
    return Scene_FindEntityById((Scene*)scene, entity_id);
}
