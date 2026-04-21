#ifndef JIMOKOMI_ENGINE_SCENE_ENTITY_H
#define JIMOKOMI_ENGINE_SCENE_ENTITY_H

#include "Component.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Scene;

typedef enum EntityDirtyFlags
{
    ENTITY_DIRTY_NONE = 0,
    ENTITY_DIRTY_VISIBILITY = 1 << 0,
    ENTITY_DIRTY_APPEARED = 1 << 1,
    ENTITY_DIRTY_REMOVED = 1 << 2,
    ENTITY_DIRTY_ENABLED = 1 << 3,
    ENTITY_DIRTY_DISABLED = 1 << 4,
    ENTITY_DIRTY_LAYER_SORT = 1 << 5,
    ENTITY_DIRTY_VISUAL_STATE = 1 << 6,
    ENTITY_DIRTY_SELECTION_HIGHLIGHT = 1 << 7
} EntityDirtyFlags;

typedef struct Entity Entity;

void Entity_Init(Entity* entity, uint32_t id);
Entity* Entity_Create(uint32_t id);
void Entity_Destroy(Entity* entity);

bool Entity_AddComponent(Entity* entity, Component* component);
Component* Entity_GetComponent(const Entity* entity, ComponentType type);
Component* Entity_RemoveComponent(Entity* entity, ComponentType type);
bool Entity_HasComponent(const Entity* entity, ComponentType type);
uint32_t Entity_GetId(const Entity* entity);
bool Entity_IsActive(const Entity* entity);
void* Entity_GetUserData(const Entity* entity);
void Entity_SetUserData(Entity* entity, void* user_data);
void Entity_SetActive(Entity* entity, bool active);
void Entity_MarkDirty(Entity* entity, uint32_t dirty_flags);
void Entity_ClearDirty(Entity* entity, uint32_t dirty_flags);
bool Entity_IsDirty(const Entity* entity, uint32_t dirty_flags);

#endif
