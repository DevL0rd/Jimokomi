#ifndef JIMOKOMI_ENGINE_SCENE_ENTITY_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_ENTITY_INTERNAL_H

#include "Entity.h"

struct Entity
{
    uint32_t id;
    bool active;
    uint32_t dirty_flags;
    int spatial_grid_entry_index;
    struct Scene* scene;
    void* user_data;
    Component** components;
    Component* component_slots[COMPONENT_RANDOM_FORCE + 1];
    size_t component_count;
    size_t component_capacity;
};

static inline Component* Entity_GetFixedComponent(const Entity* entity, ComponentType type)
{
    if (entity == NULL || (size_t)type >= (sizeof(entity->component_slots) / sizeof(entity->component_slots[0]))) {
        return NULL;
    }

    return entity->component_slots[type];
}

#endif
