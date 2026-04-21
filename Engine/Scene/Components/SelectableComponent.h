#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_SELECTABLECOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_SELECTABLECOMPONENT_H

#include "../Component.h"

typedef struct SelectableComponent
{
    Component base;
    bool enabled;
    bool selected;
    bool hovered;
    uint32_t state_version;
} SelectableComponent;

void SelectableComponent_Init(SelectableComponent* component);
SelectableComponent* SelectableComponent_Create(void);
void SelectableComponent_Destroy(SelectableComponent* component);
void SelectableComponent_SetState(SelectableComponent* component, bool selected, bool hovered);

#endif
