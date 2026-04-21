#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_STORAGE_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_STORAGE_INTERNAL_H

#include "SceneInternal.h"

struct SceneStorage
{
    struct Entity** entities;
    struct Entity** dynamic_entities;
    struct Entity** renderable_entities;
    struct Entity** random_force_entities;
    size_t entity_count;
    size_t entity_capacity;
    size_t dynamic_entity_count;
    size_t dynamic_entity_capacity;
    size_t renderable_entity_count;
    size_t renderable_entity_capacity;
    size_t random_force_entity_capacity;
    size_t random_force_entity_count;
};

#endif
