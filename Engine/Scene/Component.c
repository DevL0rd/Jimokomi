#include "Component.h"

void Component_Init(Component* component, ComponentType type, void (*destroy)(Component* component))
{
    if (component == NULL)
    {
        return;
    }

    component->type = type;
    component->entity = NULL;
    component->destroy = destroy;
}

void Component_Dispose(Component* component)
{
    if (component == NULL)
    {
        return;
    }

    component->entity = NULL;
}
