#include "InteractionSystem.h"

#include "../Physics/PhysicsBodyControl.h"
#include "../Physics/PhysicsWorld.h"
#include "../Scene/Components/DraggableComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Entity.h"
#include "../Scene/Scene.h"
#include "../Scene/ScenePhysics.h"

void InteractionSystem_ApplyDragPacket(Scene* scene, const EngineRuntimeInputPacket* input_packet)
{
    PhysicsWorld* physics_world;
    Entity* entity;
    RigidBodyComponent* rigid_body;
    DraggableComponent* draggable;
    float release_velocity_scale = 0.55f;

    if (scene == NULL || input_packet == NULL ||
        (!input_packet->drag_entity_active && !input_packet->drag_entity_release) ||
        input_packet->drag_entity_id == 0U)
    {
        return;
    }

    physics_world = Scene_GetPhysicsWorld(scene);
    entity = Scene_FindEntityById(scene, input_packet->drag_entity_id);
    if (physics_world == NULL || entity == NULL)
    {
        return;
    }

    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    draggable = (DraggableComponent*)Entity_GetComponent(entity, COMPONENT_DRAGGABLE);
    if (rigid_body == NULL || draggable == NULL || !draggable->enabled)
    {
        return;
    }
    release_velocity_scale = draggable->release_velocity_scale;

    if (input_packet->drag_entity_active)
    {
        if (rigid_body->has_body)
        {
            PhysicsWorld_SetEntityAwake(physics_world, entity, true);
        }
        PhysicsWorld_SetEntityPosition(physics_world, entity, input_packet->drag_world_x, input_packet->drag_world_y);
        if (rigid_body->has_body)
        {
            PhysicsWorld_SetEntityLinearVelocity(physics_world, entity, (Vec2){ 0.0f, 0.0f });
            PhysicsWorld_SetEntityAngularVelocity(physics_world, entity, 0.0f);
        }
    }

    if (input_packet->drag_entity_release && rigid_body->has_body)
    {
        PhysicsWorld_SetEntityAwake(physics_world, entity, true);
        PhysicsWorld_SetEntityLinearVelocity(
            physics_world,
            entity,
            (Vec2){
                input_packet->drag_linear_velocity_x * release_velocity_scale,
                input_packet->drag_linear_velocity_y * release_velocity_scale
            }
        );
    }
}
