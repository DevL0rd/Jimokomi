#ifndef JIMOKOMI_GAME_LIQUID_SOURCE_SYSTEM_H
#define JIMOKOMI_GAME_LIQUID_SOURCE_SYSTEM_H

#include "LiquidVisualResources.h"
#include "../Engine/Physics/PhysicsParticles.h"
#include "../Engine/Scene/Scene.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct LiquidSourceSystemState
{
    PhysicsParticleSystemHandle particle_system;
    LiquidVisualResourceHandles visuals;
    double emit_accumulator_seconds;
    uint32_t emit_sequence;
    bool particle_system_created;
} LiquidSourceSystemState;

bool game_liquid_sources_register_resources(struct Renderer* renderer, LiquidSourceSystemState* liquid);
bool game_liquid_sources_create(Scene* scene, LiquidSourceSystemState* liquid);
void game_liquid_sources_update(Scene* scene, LiquidSourceSystemState* liquid, double dt_seconds);
void game_liquid_sources_dispose(LiquidSourceSystemState* liquid);

#endif
