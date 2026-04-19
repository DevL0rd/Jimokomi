#include "Entity.h"

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
    entity->scene = NULL;
    entity->components = NULL;
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
    component->entity = entity;
    return true;
}

Component* Entity_GetComponent(const Entity* entity, ComponentType type)
{
    size_t index = 0;

    if (entity == NULL)
    {
        return NULL;
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

    for (index = 0; index < entity->component_count; ++index)
    {
        if (entity->components[index] != NULL && entity->components[index]->type == type)
        {
            component = entity->components[index];
            memmove(&entity->components[index],
                    &entity->components[index + 1],
                    sizeof(Component*) * (entity->component_count - index - 1));
            entity->component_count -= 1;
            entity->components[entity->component_count] = NULL;
            component->entity = NULL;
            return component;
        }
    }

    return NULL;
}

bool Entity_HasComponent(const Entity* entity, ComponentType type)
{
    return Entity_GetComponent(entity, type) != NULL;
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
