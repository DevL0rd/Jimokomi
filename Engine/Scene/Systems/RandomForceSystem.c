#include "RandomForceSystem.h"

#include "../SceneInternal.h"
#include "../EntityInternal.h"
#include "../Components/RandomForceComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../../Physics/PhysicsWorld.h"

#include <math.h>
#include <stdint.h>

static uint32_t random_force_motion_next_u32(uint32_t* state)
{
    uint32_t value = state != NULL ? *state : 0u;

    if (value == 0u)
    {
        value = 0x6D2B79F5u;
    }

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;

    if (state != NULL)
    {
        *state = value;
    }

    return value;
}

static float random_force_motion_next_unit(uint32_t* state)
{
    return (float)(random_force_motion_next_u32(state) & 0x00FFFFFFu) / (float)0x01000000u;
}

void RandomForceSystem_Update(struct Scene* scene, float dt_seconds)
{
    size_t index = 0U;

    if (scene == NULL || dt_seconds <= 0.0f || scene->random_force_entity_count == 0U)
    {
        return;
    }

    for (index = 0U; index < scene->random_force_entity_count; ++index)
    {
        Entity* entity = scene->random_force_entities[index];
        RandomForceComponent* random_force = NULL;
        RigidBodyComponent* rigid_body = NULL;

        if (!Entity_IsActive(entity))
        {
            continue;
        }

        random_force = (RandomForceComponent*)Entity_GetFixedComponent(entity, COMPONENT_RANDOM_FORCE);
        rigid_body = (RigidBodyComponent*)Entity_GetFixedComponent(entity, COMPONENT_RIGID_BODY);
        if (random_force == NULL || !random_force->enabled || rigid_body == NULL || rigid_body->body_type != RIGID_BODY_DYNAMIC ||
            !rigid_body->has_body)
        {
            continue;
        }

        random_force->time_remaining_seconds -= dt_seconds;
        if (random_force->time_remaining_seconds <= 0.0f)
        {
            float angle = random_force_motion_next_unit(&random_force->random_state) * 6.28318530718f;
            float magnitude = random_force->force_strength * random_force_motion_next_unit(&random_force->random_state);

            random_force->current_force_x = cosf(angle) * magnitude;
            random_force->current_force_y = sinf(angle) * magnitude;
            random_force->time_remaining_seconds = random_force->interval_seconds > 0.001f ? random_force->interval_seconds : 0.001f;
        }

        if (random_force->current_force_x != 0.0f || random_force->current_force_y != 0.0f)
        {
            PhysicsWorld_ApplyEntityForce(
                scene->physics_world,
                entity,
                (Vec2){ random_force->current_force_x, random_force->current_force_y },
                true
            );
        }
    }
}
