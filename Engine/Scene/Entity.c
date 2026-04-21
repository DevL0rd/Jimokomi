#include "EntityInternal.h"

#include <stdlib.h>
#include <string.h>

static bool Entity_ReserveComponentSlots(Entity* entity, size_t required_capacity)
{
    Component** components = NULL;
    size_t new_capacity = 0;

    if (entity == NULL)
    {
        return false;
    }

    if (entity->component_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = entity->component_capacity == 0 ? 4 : entity->component_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    components = (Component**)realloc(entity->components, sizeof(Component*) * new_capacity);
    if (components == NULL)
    {
        return false;
    }

    entity->components = components;
    entity->component_capacity = new_capacity;
    return true;
}

void Entity_Init(Entity* entity, uint32_t id)
{
    if (entity == NULL)
    {
        return;
    }

    entity->id = id;
    entity->active = true;
    entity->dirty_flags = ENTITY_DIRTY_APPEARED | ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_LAYER_SORT;
    entity->spatial_grid_entry_index = -1;
    entity->scene = NULL;
    entity->user_data = NULL;
    entity->components = NULL;
    memset(entity->component_slots, 0, sizeof(entity->component_slots));
    entity->component_count = 0;
    entity->component_capacity = 0;
}

Entity* Entity_Create(uint32_t id)
{
    Entity* entity = (Entity*)calloc(1, sizeof(Entity));
    if (entity == NULL)
    {
        return NULL;
    }

    Entity_Init(entity, id);
    return entity;
}

bool Entity_AddComponent(Entity* entity, Component* component)
{
    size_t index = 0;

    if (entity == NULL || component == NULL)
    {
        return false;
    }

    for (index = 0; index < entity->component_count; ++index)
    {
        if (entity->components[index] != NULL && entity->components[index]->type == component->type)
        {
            Entity_RemoveComponent(entity, component->type);
            break;
        }
    }

    if (!Entity_ReserveComponentSlots(entity, entity->component_count + 1))
    {
        return false;
    }

    entity->components[entity->component_count++] = component;
    if ((size_t)component->type < (sizeof(entity->component_slots) / sizeof(entity->component_slots[0])))
    {
        entity->component_slots[component->type] = component;
    }
    component->entity = entity;
    Entity_MarkDirty(entity, ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_LAYER_SORT);
    return true;
}

Component* Entity_GetComponent(const Entity* entity, ComponentType type)
{
    size_t index = 0;

    if (entity == NULL)
    {
        return NULL;
    }

    if ((size_t)type < (sizeof(entity->component_slots) / sizeof(entity->component_slots[0])))
    {
        return entity->component_slots[type];
    }

    for (index = 0; index < entity->component_count; ++index)
    {
        if (entity->components[index] != NULL && entity->components[index]->type == type)
        {
            return entity->components[index];
        }
    }

    return NULL;
}

Component* Entity_RemoveComponent(Entity* entity, ComponentType type)
{
    size_t index = 0;
    Component* component = NULL;

    if (entity == NULL)
    {
        return NULL;
    }

    if ((size_t)type < (sizeof(entity->component_slots) / sizeof(entity->component_slots[0])))
    {
        component = entity->component_slots[type];
    }

    if (component == NULL)
    {
        return NULL;
    }

    for (index = 0; index < entity->component_count; ++index)
    {
        if (entity->components[index] == component)
        {
            memmove(&entity->components[index],
                    &entity->components[index + 1],
                    sizeof(Component*) * (entity->component_count - index - 1));
            entity->component_count -= 1;
            entity->components[entity->component_count] = NULL;
            if ((size_t)type < (sizeof(entity->component_slots) / sizeof(entity->component_slots[0])))
            {
                entity->component_slots[type] = NULL;
            }
            component->entity = NULL;
            Entity_MarkDirty(entity, ENTITY_DIRTY_VISIBILITY | ENTITY_DIRTY_LAYER_SORT);
            return component;
        }
    }

    return NULL;
}

bool Entity_HasComponent(const Entity* entity, ComponentType type)
{
    return Entity_GetComponent(entity, type) != NULL;
}

uint32_t Entity_GetId(const Entity* entity)
{
    return entity != NULL ? entity->id : 0U;
}

bool Entity_IsActive(const Entity* entity)
{
    return entity != NULL && entity->active;
}

void* Entity_GetUserData(const Entity* entity)
{
    return entity != NULL ? entity->user_data : NULL;
}

void Entity_SetUserData(Entity* entity, void* user_data)
{
    if (entity == NULL)
    {
        return;
    }

    entity->user_data = user_data;
}

void Entity_SetActive(Entity* entity, bool active)
{
    if (entity == NULL || entity->active == active)
    {
        return;
    }

    entity->active = active;
    Entity_MarkDirty(
        entity,
        ENTITY_DIRTY_VISIBILITY | (active ? ENTITY_DIRTY_ENABLED : ENTITY_DIRTY_DISABLED)
    );
}

void Entity_MarkDirty(Entity* entity, uint32_t dirty_flags)
{
    if (entity == NULL)
    {
        return;
    }

    entity->dirty_flags |= dirty_flags;
}

void Entity_ClearDirty(Entity* entity, uint32_t dirty_flags)
{
    if (entity == NULL)
    {
        return;
    }

    entity->dirty_flags &= ~dirty_flags;
}

bool Entity_IsDirty(const Entity* entity, uint32_t dirty_flags)
{
    if (entity == NULL)
    {
        return false;
    }

    return (entity->dirty_flags & dirty_flags) != 0U;
}

void Entity_Destroy(Entity* entity)
{
    size_t index = 0;

    if (entity == NULL)
    {
        return;
    }

    for (index = entity->component_count; index > 0; --index)
    {
        Component* component = entity->components[index - 1];
        if (component != NULL)
        {
            if (component->destroy != NULL)
            {
                component->destroy(component);
            }
            else
            {
                free(component);
            }
        }
    }

    free(entity->components);
    entity->components = NULL;
    entity->component_count = 0;
    entity->component_capacity = 0;
    entity->scene = NULL;
    entity->active = false;
    free(entity);
}
