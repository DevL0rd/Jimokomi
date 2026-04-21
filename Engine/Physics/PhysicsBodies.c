#include "PhysicsWorldEntitiesInternal.h"
#include "PhysicsWorldLifecycleInternal.h"
#include "PhysicsWorldStatsInternal.h"

#include "PhysicsBodyControl.h"

#include "../Scene/EntityInternal.h"
#include "../Scene/Components/ColliderComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Components/TransformComponent.h"

#include <math.h>

b2BodyId PhysicsWorld_LoadBodyHandle(PhysicsBodyHandle handle)
{
    return (b2BodyId){ handle.index1, handle.world0, handle.generation };
}

PhysicsBodyHandle PhysicsWorld_StoreBodyHandle(b2BodyId body_id)
{
    return (PhysicsBodyHandle){ body_id.index1, body_id.world0, body_id.generation };
}

PhysicsShapeHandle PhysicsWorld_StoreShapeHandle(b2ShapeId shape_id)
{
    return (PhysicsShapeHandle){ shape_id.index1, shape_id.world0, shape_id.generation };
}

static b2BodyType PhysicsWorld_ToBox2DType(RigidBodyType type)
{
    switch (type)
    {
        case RIGID_BODY_STATIC:
            return b2_staticBody;
        case RIGID_BODY_KINEMATIC:
            return b2_kinematicBody;
        case RIGID_BODY_DYNAMIC:
        default:
            return b2_dynamicBody;
    }
}

b2BodyId PhysicsWorld_EnsureEntityBody(PhysicsWorld* world, struct Entity* entity)
{
    TransformComponent* transform = NULL;
    RigidBodyComponent* rigid_body = NULL;
    ColliderComponent* collider = NULL;

    if (world == NULL || entity == NULL || !world->lifecycle->has_world)
    {
        return b2_nullBodyId;
    }

    transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
    rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
    collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
    if (transform == NULL || rigid_body == NULL || collider == NULL)
    {
        PhysicsWorld_RemoveBodyForEntity(world, entity);
        return b2_nullBodyId;
    }

    if (rigid_body->has_body && b2Body_IsValid(PhysicsWorld_LoadBodyHandle(rigid_body->body_id)))
    {
        return PhysicsWorld_LoadBodyHandle(rigid_body->body_id);
    }

    {
        b2BodyDef body_def = b2DefaultBodyDef();
        b2ShapeDef shape_def = b2DefaultShapeDef();
        b2BodyId body_id;

        body_def.type = PhysicsWorld_ToBox2DType(rigid_body->body_type);
        body_def.position.x = transform->x;
        body_def.position.y = transform->y;
        body_def.rotation = b2MakeRot(transform->angle_radians);
        body_def.linearVelocity.x = rigid_body->initial_velocity_x;
        body_def.linearVelocity.y = rigid_body->initial_velocity_y;
        body_def.angularVelocity = rigid_body->initial_angular_velocity;
        body_def.linearDamping = rigid_body->friction_air;
        body_def.angularDamping = rigid_body->friction_air;
        body_def.userData = entity;
        body_def.enableSleep = true;
        if (world->lifecycle->has_sleep_threshold_setting)
        {
            body_def.sleepThreshold = world->lifecycle->sleep_threshold;
        }
        body_def.motionLocks.angularZ = rigid_body->fixed_rotation;

        body_id = b2CreateBody(world->lifecycle->world_id, &body_def);
        shape_def.density = rigid_body->density;
        shape_def.material.friction = rigid_body->friction;
        shape_def.material.restitution = rigid_body->restitution;
        shape_def.isSensor = collider->is_sensor;
        shape_def.userData = entity;

        if (collider->shape == COLLIDER_SHAPE_RECT)
        {
            b2Polygon box = b2MakeBox(collider->width * 0.5f, collider->height * 0.5f);
            rigid_body->shape_id = PhysicsWorld_StoreShapeHandle(b2CreatePolygonShape(body_id, &shape_def, &box));
        }
        else
        {
            b2Circle circle;
            circle.center = (b2Vec2){0.0f, 0.0f};
            circle.radius = collider->radius;
            rigid_body->shape_id = PhysicsWorld_StoreShapeHandle(b2CreateCircleShape(body_id, &shape_def, &circle));
        }

        rigid_body->body_id = PhysicsWorld_StoreBodyHandle(body_id);
        rigid_body->has_body = true;
        rigid_body->applied_sleep_threshold = world->lifecycle->has_sleep_threshold_setting
            ? world->lifecycle->sleep_threshold
            : body_def.sleepThreshold;
        rigid_body->sleep_threshold_is_onscreen = false;
        RigidBodyComponent_ClearDirty(rigid_body, RIGID_BODY_DIRTY_DEFINITION);
        ColliderComponent_ClearDirty(collider, COLLIDER_DIRTY_SHAPE | COLLIDER_DIRTY_BOUNDS);
        return body_id;
    }
}

bool PhysicsWorld_SetEntitySleepThreshold(PhysicsWorld* world, struct Entity* entity, float sleep_threshold, bool is_onscreen)
{
    RigidBodyComponent* rigid_body = NULL;
    b2BodyId body_id;

    if (entity == NULL)
    {
        return false;
    }

    rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body == NULL || rigid_body->body_type == RIGID_BODY_STATIC)
    {
        return false;
    }
    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id))
    {
        return false;
    }
    if (fabsf(rigid_body->applied_sleep_threshold - sleep_threshold) <= 0.0001f &&
        rigid_body->sleep_threshold_is_onscreen == is_onscreen)
    {
        return true;
    }

    b2Body_SetSleepThreshold(body_id, sleep_threshold);
    rigid_body->applied_sleep_threshold = sleep_threshold;
    rigid_body->sleep_threshold_is_onscreen = is_onscreen;
    return true;
}

bool PhysicsWorld_GetEntityBody(
    PhysicsWorld* world,
    struct Entity* entity,
    bool create_if_missing,
    b2BodyId* out_body_id
) {
    RigidBodyComponent* rigid_body;
    b2BodyId body_id;

    if (out_body_id != NULL) {
        *out_body_id = b2_nullBodyId;
    }
    if (world == NULL || entity == NULL || out_body_id == NULL) {
        return false;
    }

    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body == NULL) {
        return false;
    }

    body_id = rigid_body->has_body ? PhysicsWorld_LoadBodyHandle(rigid_body->body_id) : b2_nullBodyId;
    if (!b2Body_IsValid(body_id) && create_if_missing) {
        body_id = PhysicsWorld_EnsureEntityBody(world, entity);
    }
    if (!b2Body_IsValid(body_id)) {
        return false;
    }

    *out_body_id = body_id;
    return true;
}

void PhysicsWorld_RemoveBodyForEntity(PhysicsWorld* world, struct Entity* entity)
{
    RigidBodyComponent* rigid_body = NULL;

    if (world == NULL || entity == NULL)
    {
        return;
    }

    rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
    if (rigid_body == NULL || !rigid_body->has_body || !b2Body_IsValid(PhysicsWorld_LoadBodyHandle(rigid_body->body_id)))
    {
        return;
    }

    b2DestroyBody(PhysicsWorld_LoadBodyHandle(rigid_body->body_id));
    rigid_body->body_id = (PhysicsBodyHandle){ 0 };
    rigid_body->shape_id = (PhysicsShapeHandle){ 0 };
    rigid_body->has_body = false;
    RigidBodyComponent_MarkDefinitionDirty(rigid_body);
}

void PhysicsWorld_ClearEntityBodies(PhysicsWorld* world, struct Scene* scene)
{
    size_t index = 0;

    if (world == NULL)
    {
        return;
    }

    for (index = 0; index < world->entities->tracked_entity_count; ++index)
    {
        PhysicsWorld_RemoveBodyForEntity(world, world->entities->tracked_entities[index]);
    }

    (void)scene;
}

void PhysicsWorld_SetEntityPosition(PhysicsWorld* world, struct Entity* entity, float x, float y)
{
    TransformComponent* transform = NULL;
    RigidBodyComponent* rigid_body = NULL;
    ColliderComponent* collider = NULL;

    if (entity == NULL)
    {
        return;
    }

    transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
    if (transform == NULL)
    {
        return;
    }

    transform->previous_x = x;
    transform->previous_y = y;
    transform->previous_angle_radians = transform->angle_radians;
    TransformComponent_SetPosition(transform, x, y, true);

    rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
    collider = (ColliderComponent*)Entity_GetFixedComponent(entity, COMPONENT_COLLIDER);
    if (world != NULL && rigid_body != NULL && collider != NULL)
    {
        b2BodyId body_id = rigid_body->has_body ? PhysicsWorld_LoadBodyHandle(rigid_body->body_id) : PhysicsWorld_EnsureEntityBody(world, entity);
        if (b2Body_IsValid(body_id))
        {
            b2Body_SetTransform(body_id, (b2Vec2){x, y}, b2MakeRot(transform->angle_radians));
            b2Body_SetLinearVelocity(body_id, (b2Vec2){0.0f, 0.0f});
            b2Body_SetAngularVelocity(body_id, 0.0f);
            return;
        }
    }

    TransformComponent_MarkDirty(transform, TRANSFORM_DIRTY_POSITION | TRANSFORM_DIRTY_TELEPORT);
}

void PhysicsWorld_SetEntityTargetPosition(PhysicsWorld* world, struct Entity* entity, float x, float y, float time_step, bool wake)
{
    TransformComponent* transform = NULL;
    b2BodyId body_id;

    if (entity == NULL)
    {
        return;
    }

    transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
    if (transform == NULL)
    {
        return;
    }

    transform->previous_x = transform->x;
    transform->previous_y = transform->y;
    transform->previous_angle_radians = transform->angle_radians;
    TransformComponent_SetPosition(transform, x, y, false);

    if (world == NULL || time_step <= 0.0f || !PhysicsWorld_GetEntityBody(world, entity, true, &body_id))
    {
        return;
    }

    b2Body_SetTargetTransform(
        body_id,
        (b2Transform){ { x, y }, b2MakeRot(transform->angle_radians) },
        time_step,
        wake
    );
}

bool PhysicsWorld_GetEntityLinearVelocity(PhysicsWorld* world, struct Entity* entity, Vec2* out_velocity)
{
    b2BodyId body_id;
    b2Vec2 velocity;

    if (out_velocity != NULL) {
        out_velocity->x = 0.0f;
        out_velocity->y = 0.0f;
    }
    if (out_velocity == NULL || !PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    velocity = b2Body_GetLinearVelocity(body_id);
    out_velocity->x = velocity.x;
    out_velocity->y = velocity.y;
    return true;
}

bool PhysicsWorld_SetEntityLinearVelocity(PhysicsWorld* world, struct Entity* entity, Vec2 velocity)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_SetLinearVelocity(body_id, (b2Vec2){ velocity.x, velocity.y });
    return true;
}

bool PhysicsWorld_SetEntityAngularVelocity(PhysicsWorld* world, struct Entity* entity, float angular_velocity)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_SetAngularVelocity(body_id, angular_velocity);
    return true;
}

bool PhysicsWorld_ApplyEntityForce(PhysicsWorld* world, struct Entity* entity, Vec2 force, bool wake)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_ApplyForceToCenter(body_id, (b2Vec2){ force.x, force.y }, wake);
    return true;
}

bool PhysicsWorld_ApplyEntityLinearImpulse(PhysicsWorld* world, struct Entity* entity, Vec2 impulse, bool wake)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_ApplyLinearImpulseToCenter(body_id, (b2Vec2){ impulse.x, impulse.y }, wake);
    return true;
}

bool PhysicsWorld_SetEntityAwake(PhysicsWorld* world, struct Entity* entity, bool awake)
{
    b2BodyId body_id;

    if (!PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    b2Body_SetAwake(body_id, awake);
    return true;
}

bool PhysicsWorld_IsEntityAwake(PhysicsWorld* world, struct Entity* entity, bool* out_awake)
{
    b2BodyId body_id;

    if (out_awake != NULL) {
        *out_awake = false;
    }
    if (out_awake == NULL || !PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    *out_awake = b2Body_IsAwake(body_id);
    return true;
}

bool PhysicsWorld_GetEntityContactCapacity(PhysicsWorld* world, struct Entity* entity, int* out_contact_capacity)
{
    b2BodyId body_id;

    if (out_contact_capacity != NULL) {
        *out_contact_capacity = 0;
    }
    if (out_contact_capacity == NULL || !PhysicsWorld_GetEntityBody(world, entity, false, &body_id)) {
        return false;
    }

    *out_contact_capacity = b2Body_GetContactCapacity(body_id);
    return true;
}
