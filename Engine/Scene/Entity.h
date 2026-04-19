#ifndef JIMOKOMI_ENGINE_SCENE_ENTITY_H
#define JIMOKOMI_ENGINE_SCENE_ENTITY_H

#include "Component.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Scene;

typedef struct Entity
{
    uint32_t id;
    bool active;
    struct Scene* scene;
    Component** components;
    size_t component_count;
    size_t component_capacity;
} Entity;

void Entity_Init(Entity* entity, uint32_t id);
Entity* Entity_Create(uint32_t id);
void Entity_Destroy(Entity* entity);

bool Entity_AddComponent(Entity* entity, Component* component);
Component* Entity_GetComponent(const Entity* entity, ComponentType type);
Component* Entity_RemoveComponent(Entity* entity, ComponentType type);
bool Entity_HasComponent(const Entity* entity, ComponentType type);

#endif
