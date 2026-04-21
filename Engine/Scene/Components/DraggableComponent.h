#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_DRAGGABLECOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_DRAGGABLECOMPONENT_H

#include "../Component.h"

typedef struct DraggableComponent
{
    Component base;
    bool enabled;
    float pick_radius;
    float release_velocity_scale;
} DraggableComponent;

void DraggableComponent_Init(DraggableComponent* component, float pick_radius);
DraggableComponent* DraggableComponent_Create(float pick_radius);
void DraggableComponent_Destroy(DraggableComponent* component);

#endif
