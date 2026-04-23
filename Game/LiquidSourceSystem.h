#ifndef JIMOKOMI_GAME_LIQUID_SOURCE_SYSTEM_H
#define JIMOKOMI_GAME_LIQUID_SOURCE_SYSTEM_H

#include "../Engine/Physics/PhysicsParticles.h"
#include "../Engine/Rendering/ParticleVisualResources.h"
#include "../Engine/Scene/Scene.h"

#include <stdbool.h>

typedef struct LiquidSourceSystemState
{
    PhysicsParticleSystemHandle particle_system;
    ParticleVisualResourceHandles visuals;
    bool particle_system_created;
} LiquidSourceSystemState;

bool game_liquid_sources_register_resources(struct Renderer* renderer, LiquidSourceSystemState* liquid);
bool game_liquid_sources_create(Scene* scene, LiquidSourceSystemState* liquid);
void game_liquid_sources_update(Scene* scene, LiquidSourceSystemState* liquid, double dt_seconds);
void game_liquid_sources_dispose(LiquidSourceSystemState* liquid);

#endif
