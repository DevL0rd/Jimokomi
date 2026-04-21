#include "RenderableComponent.h"

#include "../Entity.h"

#include <stdlib.h>

static void RenderableComponent_DestroyBase(Component* component)
{
    RenderableComponent_Destroy((RenderableComponent*)component);
}

void RenderableComponent_Init(RenderableComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_RENDERABLE, RenderableComponent_DestroyBase);
    component->visual_source_handle = (ResourceHandle){ 0U };
    component->material_handle = (ResourceHandle){ 0U };
    component->shader_handle = (ResourceHandle){ 0U };
    component->anchor_x = 0.5f;
    component->anchor_y = 0.5f;
    component->layer = 0;
    component->visible = true;
}

RenderableComponent* RenderableComponent_Create(void)
{
    RenderableComponent* component = (RenderableComponent*)calloc(1, sizeof(*component));
    if (component == NULL)
    {
        return NULL;
    }

    RenderableComponent_Init(component);
    return component;
}

RenderableComponent* RenderableComponent_CreateWithDesc(const RenderableComponentDesc* desc)
{
    RenderableComponent* component = RenderableComponent_Create();
    if (component == NULL || desc == NULL)
    {
        return component;
    }

    component->visual_source_handle = desc->visual_source_handle;
    component->material_handle = desc->material_handle;
    component->shader_handle = desc->shader_handle;
    component->anchor_x = desc->anchor_x;
    component->anchor_y = desc->anchor_y;
    component->layer = desc->layer;
    component->visible = desc->visible;
    return component;
}

void RenderableComponent_Destroy(RenderableComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}
