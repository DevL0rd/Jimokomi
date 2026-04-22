#include "SceneFactories.h"

#include "SceneInternal.h"
#include "Entity.h"
#include "Components/ColliderComponent.h"
#include "Components/DraggableComponent.h"
#include "Components/RenderableComponent.h"
#include "Components/RigidBodyComponent.h"
#include "Components/SelectableComponent.h"
#include "Components/TransformComponent.h"

struct Entity* Scene_CreateDynamicCircle(Scene* scene, const SceneDynamicCircleDesc* desc)
{
    Entity* entity = NULL;
    TransformComponent* transform = NULL;
    RigidBodyComponent* rigid_body = NULL;
    ColliderComponent* collider = NULL;
    RenderableComponent* renderable = NULL;
    SelectableComponent* selectable = NULL;
    DraggableComponent* draggable = NULL;

    if (scene == NULL || desc == NULL || desc->radius <= 0.0f)
    {
        return NULL;
    }

    entity = Entity_Create(0U);
    transform = TransformComponent_Create(desc->x, desc->y, 0.0f);
    rigid_body = RigidBodyComponent_CreateWithDesc(&(RigidBodyComponentDesc){
        .body_type = RIGID_BODY_DYNAMIC,
        .fixed_rotation = false,
        .density = desc->density,
        .friction_air = 0.01f,
        .friction = desc->friction,
        .restitution = desc->restitution,
        .initial_velocity_x = desc->initial_velocity.x,
        .initial_velocity_y = desc->initial_velocity.y,
        .initial_angular_velocity = desc->initial_angular_velocity
    });
    collider = ColliderComponent_CreateCircle(desc->radius);
    renderable = RenderableComponent_CreateWithDesc(&(RenderableComponentDesc){
        .procedural_texture_handle = desc->procedural_texture_handle,
        .material_handle = desc->material_handle,
        .shader_handle = desc->shader_handle,
        .anchor_x = 0.5f,
        .anchor_y = 0.5f,
        .tint = desc->tint.value != 0U ? desc->tint : (Color32){ 0xffffffffU },
        .layer = 0,
        .visible = true
    });
    selectable = desc->selectable ? SelectableComponent_Create() : NULL;
    draggable = desc->draggable
        ? DraggableComponent_CreateWithDesc(&(DraggableComponentDesc){
            .enabled = true,
            .pick_radius = desc->drag_pick_radius > 0.0f ? desc->drag_pick_radius : desc->radius,
            .release_velocity_scale = 0.55f
        })
        : NULL;
    if (entity == NULL || transform == NULL || rigid_body == NULL || collider == NULL ||
        renderable == NULL || (desc->selectable && selectable == NULL) || (desc->draggable && draggable == NULL))
    {
        Entity_Destroy(entity);
        TransformComponent_Destroy(transform);
        RigidBodyComponent_Destroy(rigid_body);
        ColliderComponent_Destroy(collider);
        RenderableComponent_Destroy(renderable);
        SelectableComponent_Destroy(selectable);
        DraggableComponent_Destroy(draggable);
        return NULL;
    }

    Entity_AddComponent(entity, &transform->base);
    Entity_AddComponent(entity, &rigid_body->base);
    Entity_AddComponent(entity, &collider->base);
    Entity_AddComponent(entity, &renderable->base);
    if (selectable != NULL)
    {
        Entity_AddComponent(entity, &selectable->base);
    }
    if (draggable != NULL)
    {
        Entity_AddComponent(entity, &draggable->base);
    }
    if (!Scene_AddEntity(scene, entity))
    {
        Entity_Destroy(entity);
        return NULL;
    }

    return entity;
}

struct Entity* Scene_CreateVisualCircle(Scene* scene, const SceneVisualCircleDesc* desc)
{
    Entity* entity = NULL;
    TransformComponent* transform = NULL;
    ColliderComponent* collider = NULL;
    RenderableComponent* renderable = NULL;

    if (scene == NULL || desc == NULL || desc->radius <= 0.0f)
    {
        return NULL;
    }

    entity = Entity_Create(0U);
    transform = TransformComponent_Create(desc->x, desc->y, 0.0f);
    collider = ColliderComponent_CreateCircle(desc->radius);
    renderable = RenderableComponent_CreateWithDesc(&(RenderableComponentDesc){
        .procedural_texture_handle = desc->procedural_texture_handle,
        .material_handle = desc->material_handle,
        .shader_handle = desc->shader_handle,
        .anchor_x = 0.5f,
        .anchor_y = 0.5f,
        .tint = desc->tint.value != 0U ? desc->tint : (Color32){ 0xffffffffU },
        .layer = desc->layer,
        .visible = desc->visible
    });
    if (entity == NULL || transform == NULL || collider == NULL || renderable == NULL)
    {
        Entity_Destroy(entity);
        TransformComponent_Destroy(transform);
        ColliderComponent_Destroy(collider);
        RenderableComponent_Destroy(renderable);
        return NULL;
    }

    Entity_AddComponent(entity, &transform->base);
    Entity_AddComponent(entity, &collider->base);
    Entity_AddComponent(entity, &renderable->base);
    if (!Scene_AddEntity(scene, entity))
    {
        Entity_Destroy(entity);
        return NULL;
    }

    return entity;
}

struct Entity* Scene_CreateKinematicBox(Scene* scene, float x, float y, float width, float height)
{
    Entity* entity = NULL;
    TransformComponent* transform = NULL;
    RigidBodyComponent* rigid_body = NULL;
    ColliderComponent* collider = NULL;

    if (scene == NULL || width <= 0.0f || height <= 0.0f)
    {
        return NULL;
    }

    entity = Entity_Create(0U);
    transform = TransformComponent_Create(x, y, 0.0f);
    rigid_body = RigidBodyComponent_CreateWithDesc(&(RigidBodyComponentDesc){
        .body_type = RIGID_BODY_KINEMATIC,
        .fixed_rotation = false,
        .density = 0.001f,
        .friction_air = 0.01f,
        .friction = 0.1f,
        .restitution = 0.0f
    });
    collider = ColliderComponent_CreateRect(width, height);
    if (entity == NULL || transform == NULL || rigid_body == NULL || collider == NULL)
    {
        Entity_Destroy(entity);
        TransformComponent_Destroy(transform);
        RigidBodyComponent_Destroy(rigid_body);
        ColliderComponent_Destroy(collider);
        return NULL;
    }

    Entity_AddComponent(entity, &transform->base);
    Entity_AddComponent(entity, &rigid_body->base);
    Entity_AddComponent(entity, &collider->base);
    if (!Scene_AddEntity(scene, entity))
    {
        Entity_Destroy(entity);
        return NULL;
    }

    return entity;
}

struct Entity* Scene_CreateStaticBox(Scene* scene, float x, float y, float width, float height)
{
    Entity* entity = NULL;
    TransformComponent* transform = NULL;
    RigidBodyComponent* rigid_body = NULL;
    ColliderComponent* collider = NULL;

    if (scene == NULL || width <= 0.0f || height <= 0.0f)
    {
        return NULL;
    }

    entity = Entity_Create(0U);
    transform = TransformComponent_Create(x, y, 0.0f);
    rigid_body = RigidBodyComponent_CreateWithDesc(&(RigidBodyComponentDesc){
        .body_type = RIGID_BODY_STATIC,
        .fixed_rotation = false,
        .density = 0.001f,
        .friction_air = 0.01f,
        .friction = 0.1f,
        .restitution = 0.0f
    });
    collider = ColliderComponent_CreateRect(width, height);
    if (entity == NULL || transform == NULL || rigid_body == NULL || collider == NULL)
    {
        Entity_Destroy(entity);
        TransformComponent_Destroy(transform);
        RigidBodyComponent_Destroy(rigid_body);
        ColliderComponent_Destroy(collider);
        return NULL;
    }

    Entity_AddComponent(entity, &transform->base);
    Entity_AddComponent(entity, &rigid_body->base);
    Entity_AddComponent(entity, &collider->base);
    if (!Scene_AddEntity(scene, entity))
    {
        Entity_Destroy(entity);
        return NULL;
    }

    return entity;
}

bool Scene_AddBoundsColliders(Scene* scene, Rect bounds, float thickness)
{
    if (scene == NULL || bounds.w <= 0.0f || bounds.h <= 0.0f || thickness <= 0.0f)
    {
        return false;
    }

    return Scene_CreateStaticBox(scene, bounds.x + bounds.w * 0.5f, bounds.y - thickness * 0.5f, bounds.w, thickness) != NULL &&
           Scene_CreateStaticBox(scene, bounds.x + bounds.w * 0.5f, bounds.y + bounds.h + thickness * 0.5f, bounds.w, thickness) != NULL &&
           Scene_CreateStaticBox(scene, bounds.x - thickness * 0.5f, bounds.y + bounds.h * 0.5f, thickness, bounds.h) != NULL &&
           Scene_CreateStaticBox(scene, bounds.x + bounds.w + thickness * 0.5f, bounds.y + bounds.h * 0.5f, thickness, bounds.h) != NULL;
}
