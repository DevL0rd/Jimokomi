#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Entity;

typedef enum ComponentType
{
    COMPONENT_UNKNOWN = 0,
    COMPONENT_TRANSFORM,
    COMPONENT_RIGID_BODY,
    COMPONENT_COLLIDER,
    COMPONENT_CAMERA_TARGET,
    COMPONENT_RANDOM_FORCE,
    COMPONENT_RENDERABLE,
    COMPONENT_SELECTABLE,
    COMPONENT_DRAGGABLE,
    COMPONENT_CUSTOM_BASE = 1024
} ComponentType;

typedef struct Component
{
    ComponentType type;
    struct Entity* entity;
    void (*destroy)(struct Component* component);
} Component;

void Component_Init(Component* component, ComponentType type, void (*destroy)(Component* component));
void Component_Dispose(Component* component);

#endif
