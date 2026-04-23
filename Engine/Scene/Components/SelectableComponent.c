#include "SelectableComponent.h"

#include "../Entity.h"

#include "../../Core/Memory.h"

#include <stdlib.h>

static void SelectableComponent_DestroyBase(Component* component)
{
    SelectableComponent_Destroy((SelectableComponent*)component);
}

void SelectableComponent_Init(SelectableComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_SELECTABLE, SelectableComponent_DestroyBase);
    component->enabled = true;
    component->selected = false;
    component->hovered = false;
    component->state_version = 0U;
}

SelectableComponent* SelectableComponent_Create(void)
{
    SelectableComponent* component = (SelectableComponent*)calloc(1, sizeof(*component));
    if (component == NULL)
    {
        return NULL;
    }

    SelectableComponent_Init(component);
    return component;
}

void SelectableComponent_Destroy(SelectableComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}

void SelectableComponent_SetState(SelectableComponent* component, bool selected, bool hovered)
{
    if (component == NULL || (component->selected == selected && component->hovered == hovered))
    {
        return;
    }

    component->selected = selected;
    component->hovered = hovered;
    component->state_version += 1U;
    if (component->base.entity != NULL)
    {
        Entity_MarkDirty(component->base.entity, ENTITY_DIRTY_SELECTION_HIGHLIGHT);
    }
}
