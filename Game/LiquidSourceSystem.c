#include "LiquidSourceSystem.h"

#include "GameConfig.h"
#include "../Engine/Scene/SceneParticleVisuals.h"
#include "../Engine/Scene/ScenePhysics.h"
#include <string.h>

#define LIQUID_BLUE_ARGB 0xd02f8dffU
#define LIQUID_RED_ARGB 0xd0ff4f5fU

static bool game_liquid_sources_emit_particle(
    Scene* scene,
    LiquidSourceSystemState* liquid,
    float x,
    float y,
    float velocity_y,
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
    particle.velocity = (Vec2){ 0.0f, velocity_y };
    particle.color_argb = color_argb;
    particle.lifetime_seconds = LIQUID_PARTICLE_LIFETIME_SECONDS;
    return PhysicsWorld_CreateParticle(
        Scene_GetPhysicsWorld(scene),
        liquid->particle_system,
        &particle,
        NULL
    );
}

static void game_liquid_sources_emit(Scene* scene, LiquidSourceSystemState* liquid)
{
    float left_x = WORLD_WIDTH * 0.5f - LIQUID_SOURCE_CENTER_GAP * 0.5f;
    float right_x = WORLD_WIDTH * 0.5f + LIQUID_SOURCE_CENTER_GAP * 0.5f;
    float first_offset =
        -((float)(LIQUID_SOURCE_PARTICLES_PER_SIDE - 1U) * LIQUID_SOURCE_PARTICLE_SPACING * 0.5f);
    size_t index = 0U;

    for (index = 0U; index < LIQUID_SOURCE_PARTICLES_PER_SIDE; ++index)
    {
        float x_offset = first_offset + (float)index * LIQUID_SOURCE_PARTICLE_SPACING;
        (void)game_liquid_sources_emit_particle(
            scene,
            liquid,
            left_x + x_offset,
            LIQUID_SOURCE_Y,
            LIQUID_SOURCE_SPEED,
            LIQUID_BLUE_ARGB
        );
        (void)game_liquid_sources_emit_particle(
            scene,
            liquid,
            right_x + x_offset,
            LIQUID_SOURCE_Y,
            LIQUID_SOURCE_SPEED,
            LIQUID_RED_ARGB
        );
    }
}

bool game_liquid_sources_register_resources(Renderer* renderer, LiquidSourceSystemState* liquid)
{
    return renderer_register_default_particle_visual_resources(
        renderer,
        liquid != NULL ? &liquid->visuals : NULL
    );
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
    desc.iteration_count = LIQUID_PARTICLE_STEP_ITERATIONS;
    desc.destroy_by_age = false;

    liquid->particle_system_created = PhysicsWorld_CreateParticleSystem(
        physics_world,
        &desc,
        &liquid->particle_system
    );
    if (!liquid->particle_system_created)
    {
        return false;
    }

    if (!Scene_SetDefaultParticleVisualResources(scene, &liquid->visuals))
    {
        PhysicsWorld_DestroyParticleSystem(physics_world, liquid->particle_system);
        memset(&liquid->particle_system, 0, sizeof(liquid->particle_system));
        liquid->particle_system_created = false;
        return false;
    }

    if (!Scene_RegisterParticleVisual(
        scene,
        &(SceneParticleVisualDesc){
            .particle_system = liquid->particle_system,
            .particles_visible = false,
            .mesh_visible = true,
        }
    ))
    {
        PhysicsWorld_DestroyParticleSystem(physics_world, liquid->particle_system);
        memset(&liquid->particle_system, 0, sizeof(liquid->particle_system));
        liquid->particle_system_created = false;
        return false;
    }

    return true;
}

void game_liquid_sources_update(Scene* scene, LiquidSourceSystemState* liquid, double dt_seconds)
{
    PhysicsWorld* physics_world = NULL;

    if (scene == NULL || liquid == NULL || !liquid->particle_system_created)
    {
        return;
    }

    physics_world = Scene_GetPhysicsWorld(scene);
    if (physics_world == NULL)
    {
        return;
    }

    if (dt_seconds > 0.0 &&
        PhysicsWorld_GetParticleSystemParticleCount(physics_world, liquid->particle_system) < LIQUID_PARTICLE_SOURCE_MAX_COUNT)
    {
        game_liquid_sources_emit(scene, liquid);
    }
}

void game_liquid_sources_dispose(LiquidSourceSystemState* liquid)
{
    if (liquid == NULL)
    {
        return;
    }

    memset(liquid, 0, sizeof(*liquid));
}
