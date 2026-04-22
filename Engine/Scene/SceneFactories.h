#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_FACTORIES_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_FACTORIES_H

#include "Scene.h"
#include "../Rendering/RenderCommon.h"
#include "../Rendering/ResourceTypes.h"

typedef struct SceneDynamicCircleDesc
{
    float x;
    float y;
    float radius;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    Color32 tint;
    Vec2 initial_velocity;
    float initial_angular_velocity;
    float density;
    float friction;
    float restitution;
    bool selectable;
    bool draggable;
    float drag_pick_radius;
} SceneDynamicCircleDesc;

typedef struct SceneVisualCircleDesc
{
    float x;
    float y;
    float radius;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    Color32 tint;
    int layer;
    bool visible;
} SceneVisualCircleDesc;

struct Entity* Scene_CreateDynamicCircle(Scene* scene, const SceneDynamicCircleDesc* desc);
struct Entity* Scene_CreateVisualCircle(Scene* scene, const SceneVisualCircleDesc* desc);
struct Entity* Scene_CreateStaticBox(Scene* scene, float x, float y, float width, float height);
bool Scene_AddBoundsColliders(Scene* scene, Rect bounds, float thickness);

#endif
