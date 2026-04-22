#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_RENDERABLECOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_RENDERABLECOMPONENT_H

#include "../Component.h"
#include "../../Rendering/RenderCommon.h"
#include "../../Rendering/ResourceTypes.h"

typedef struct RenderableComponent
{
    Component base;
    ResourceHandle procedural_texture_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    float anchor_x;
    float anchor_y;
    Color32 tint;
    int layer;
    bool visible;
} RenderableComponent;

typedef struct RenderableComponentDesc
{
    ResourceHandle procedural_texture_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    float anchor_x;
    float anchor_y;
    Color32 tint;
    int layer;
    bool visible;
} RenderableComponentDesc;

void RenderableComponent_Init(RenderableComponent* component);
RenderableComponent* RenderableComponent_Create(void);
RenderableComponent* RenderableComponent_CreateWithDesc(const RenderableComponentDesc* desc);
void RenderableComponent_Destroy(RenderableComponent* component);

#endif
