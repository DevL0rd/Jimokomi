#include "LiquidSourceSystem.h"

#include "GameConfig.h"
#include "../Engine/Scene/SceneFactories.h"
#include "../Engine/Scene/ScenePhysics.h"
#include "../Engine/Scene/Components/RenderableComponent.h"
#include "../Engine/Scene/Components/TransformComponent.h"

#include <stdlib.h>
#include <string.h>

#define LIQUID_BLUE_ARGB 0xd02f8dffU
#define LIQUID_RED_ARGB 0xd0ff4f5fU

static bool game_liquid_sources_reserve_particle_data(LiquidSourceSystemState* liquid, size_t required_capacity)
{
    PhysicsParticleRenderData* resized = NULL;
    size_t next_capacity = 0U;

    if (liquid == NULL)
    {
        return false;
    }
    if (liquid->particle_data_capacity >= required_capacity)
    {
        return true;
    }

    next_capacity = liquid->particle_data_capacity > 0U ? liquid->particle_data_capacity : 256U;
    while (next_capacity < required_capacity)
    {
        next_capacity *= 2U;
    }

    resized = (PhysicsParticleRenderData*)realloc(
        liquid->particle_data,
        next_capacity * sizeof(*liquid->particle_data)
    );
    if (resized == NULL)
    {
        return false;
    }

    liquid->particle_data = resized;
    liquid->particle_data_capacity = next_capacity;
    return true;
}

static bool game_liquid_sources_reserve_visual_entities(
    Scene* scene,
    LiquidSourceSystemState* liquid,
    size_t required_capacity
)
{
    Entity** resized = NULL;
    size_t next_capacity = 0U;
    size_t index = 0U;

    if (scene == NULL || liquid == NULL)
    {
        return false;
    }
    if (liquid->visual_entity_capacity < required_capacity)
    {
        next_capacity = liquid->visual_entity_capacity > 0U ? liquid->visual_entity_capacity : 256U;
        while (next_capacity < required_capacity)
        {
            next_capacity *= 2U;
        }

        resized = (Entity**)realloc(liquid->visual_entities, next_capacity * sizeof(*liquid->visual_entities));
        if (resized == NULL)
        {
            return false;
        }

        liquid->visual_entities = resized;
        for (index = liquid->visual_entity_capacity; index < next_capacity; ++index)
        {
            liquid->visual_entities[index] = NULL;
        }
        liquid->visual_entity_capacity = next_capacity;
    }

    for (index = liquid->visual_entity_count; index < required_capacity; ++index)
    {
        Entity* entity = Scene_CreateVisualCircle(
            scene,
            &(SceneVisualCircleDesc){
                .x = -LIQUID_PARTICLE_RADIUS * 4.0f,
                .y = -LIQUID_PARTICLE_RADIUS * 4.0f,
                .radius = LIQUID_PARTICLE_RADIUS,
                .visual_source_handle = liquid->visuals.source_handle,
                .material_handle = liquid->visuals.material_handle,
                .shader_handle = liquid->visuals.shader_handle,
                .tint = (Color32){ LIQUID_BLUE_ARGB },
                .layer = -1,
                .visible = false
            }
        );
        if (entity == NULL)
        {
            return false;
        }

        Entity_SetActive(entity, false);
        liquid->visual_entities[index] = entity;
        liquid->visual_entity_count += 1U;
    }

    return true;
}

static void game_liquid_sources_set_visual(
    Entity* entity,
    const PhysicsParticleRenderData* particle,
    ResourceHandle material_handle,
    Color32 tint
)
{
    TransformComponent* transform = NULL;
    RenderableComponent* renderable = NULL;

    if (entity == NULL || particle == NULL)
    {
        return;
    }

    transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
    renderable = (RenderableComponent*)Entity_GetComponent(entity, COMPONENT_RENDERABLE);
    if (transform != NULL)
    {
        transform->previous_x = particle->position.x;
        transform->previous_y = particle->position.y;
        transform->x = particle->position.x;
        transform->y = particle->position.y;
        TransformComponent_MarkDirty(transform, TRANSFORM_DIRTY_POSITION | TRANSFORM_DIRTY_TELEPORT);
    }
    if (renderable != NULL)
    {
        if (renderable->material_handle.id != material_handle.id)
        {
            renderable->material_handle = material_handle;
            Entity_MarkDirty(entity, ENTITY_DIRTY_VISUAL_STATE);
        }
        if (renderable->tint.value != tint.value)
        {
            renderable->tint = tint;
            Entity_MarkDirty(entity, ENTITY_DIRTY_VISUAL_STATE);
        }
        if (!renderable->visible)
        {
            renderable->visible = true;
            Entity_MarkDirty(entity, ENTITY_DIRTY_VISIBILITY);
        }
    }
    Entity_SetActive(entity, true);
}

static void game_liquid_sources_hide_visual(Entity* entity)
{
    RenderableComponent* renderable = NULL;

    if (entity == NULL)
    {
        return;
    }

    renderable = (RenderableComponent*)Entity_GetComponent(entity, COMPONENT_RENDERABLE);
    if (renderable != NULL && renderable->visible)
    {
        renderable->visible = false;
        Entity_MarkDirty(entity, ENTITY_DIRTY_VISIBILITY);
    }
    Entity_SetActive(entity, false);
}

static bool game_liquid_sources_emit_particle(
    Scene* scene,
    LiquidSourceSystemState* liquid,
    float x,
    float y,
    float velocity_x,
    uint32_t color_argb
)
{
    PhysicsParticleDesc particle = { 0 };

    if (scene == NULL || liquid == NULL || !liquid->particle_system_created)
    {
        return false;
    }

    particle.flags = PHYSICS_PARTICLE_FLAG_LIQUID |
                     PHYSICS_PARTICLE_FLAG_COLOR_MIXING;
    particle.position = (Vec2){ x, y };
    particle.velocity = (Vec2){ velocity_x, LIQUID_SOURCE_SPEED_Y };
    particle.color_argb = color_argb;
    particle.lifetime_seconds = LIQUID_PARTICLE_LIFETIME_SECONDS;
    return PhysicsWorld_CreateParticle(
        Scene_GetPhysicsWorld(scene),
        liquid->particle_system,
        &particle,
        NULL
    );
}

static void game_liquid_sources_emit_pair(Scene* scene, LiquidSourceSystemState* liquid)
{
    float left_x = LIQUID_SOURCE_X_OFFSET;
    float right_x = WORLD_WIDTH - LIQUID_SOURCE_X_OFFSET;
    float first_y = LIQUID_SOURCE_Y -
        ((float)(LIQUID_SOURCE_PARTICLES_PER_SIDE - 1U) * LIQUID_SOURCE_VERTICAL_SPACING * 0.5f);
    size_t index = 0U;

    for (index = 0U; index < LIQUID_SOURCE_PARTICLES_PER_SIDE; ++index)
    {
        float y = first_y + (float)index * LIQUID_SOURCE_VERTICAL_SPACING;
        (void)game_liquid_sources_emit_particle(
            scene,
            liquid,
            left_x,
            y,
            LIQUID_SOURCE_SPEED_X,
            LIQUID_BLUE_ARGB
        );
        (void)game_liquid_sources_emit_particle(
            scene,
            liquid,
            right_x,
            y,
            -LIQUID_SOURCE_SPEED_X,
            LIQUID_RED_ARGB
        );
    }

    liquid->emit_sequence += 1U;
}

void game_liquid_sources_sync_visuals(Scene* scene, LiquidSourceSystemState* liquid)
{
    PhysicsWorld* physics_world = NULL;
    size_t particle_capacity = 0U;
    size_t particle_count = 0U;
    size_t index = 0U;

    if (scene == NULL || liquid == NULL || !liquid->particle_system_created)
    {
        return;
    }

    physics_world = Scene_GetPhysicsWorld(scene);
    if (physics_world == NULL)
    {
        return;
    }

    particle_capacity = PhysicsWorld_GetParticleSystemParticleCount(physics_world, liquid->particle_system);
    if (!game_liquid_sources_reserve_particle_data(liquid, particle_capacity) ||
        !game_liquid_sources_reserve_visual_entities(scene, liquid, particle_capacity))
    {
        return;
    }

    particle_count = PhysicsWorld_CopyParticleSystemRenderData(
        physics_world,
        liquid->particle_system,
        liquid->particle_data,
        particle_capacity
    );
    for (index = 0U; index < particle_count && index < liquid->visual_entity_count; ++index)
    {
        Color32 tint = (Color32){
            liquid->particle_data[index].color_argb != 0U
                ? liquid->particle_data[index].color_argb
                : LIQUID_BLUE_ARGB
        };
        game_liquid_sources_set_visual(
            liquid->visual_entities[index],
            &liquid->particle_data[index],
            liquid->visuals.material_handle,
            tint
        );
    }

    for (index = particle_count; index < liquid->visual_entity_count; ++index)
    {
        game_liquid_sources_hide_visual(liquid->visual_entities[index]);
    }
}

bool game_liquid_sources_register_resources(Renderer* renderer, LiquidSourceSystemState* liquid)
{
    return game_register_liquid_visuals(renderer, liquid != NULL ? &liquid->visuals : NULL);
}

bool game_liquid_sources_create(Scene* scene, LiquidSourceSystemState* liquid)
{
    PhysicsParticleSystemDesc desc = { 0 };
    PhysicsWorld* physics_world = NULL;

    if (scene == NULL || liquid == NULL)
    {
        return false;
    }

    physics_world = Scene_GetPhysicsWorld(scene);
    if (physics_world == NULL)
    {
        return false;
    }

    desc.radius = LIQUID_PARTICLE_RADIUS;
    desc.destroy_by_age = false;

    liquid->particle_system_created = PhysicsWorld_CreateParticleSystem(
        physics_world,
        &desc,
        &liquid->particle_system
    );
    return liquid->particle_system_created;
}

void game_liquid_sources_update(Scene* scene, LiquidSourceSystemState* liquid, double dt_seconds)
{
    if (scene == NULL || liquid == NULL || !liquid->particle_system_created)
    {
        return;
    }

    if (dt_seconds > 0.0)
    {
        liquid->emit_accumulator_seconds += dt_seconds;
        while (liquid->emit_accumulator_seconds >= LIQUID_EMIT_INTERVAL_SECONDS)
        {
            liquid->emit_accumulator_seconds -= LIQUID_EMIT_INTERVAL_SECONDS;
            game_liquid_sources_emit_pair(scene, liquid);
        }
    }
}

void game_liquid_sources_dispose(LiquidSourceSystemState* liquid)
{
    if (liquid == NULL)
    {
        return;
    }

    free(liquid->visual_entities);
    free(liquid->particle_data);
    memset(liquid, 0, sizeof(*liquid));
}
