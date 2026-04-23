#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_PARTICLE_VISUALS_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_PARTICLE_VISUALS_INTERNAL_H

#include "SceneParticleVisuals.h"

typedef struct SceneParticleVisualState
{
    ParticleVisualResourceHandles default_resources;
    SceneParticleVisualDesc* bindings;
    size_t binding_count;
    size_t binding_capacity;
} SceneParticleVisualState;

void SceneParticleVisualState_Dispose(SceneParticleVisualState* state);
const SceneParticleVisualDesc* Scene_GetParticleVisualBindings(const Scene* scene, size_t* out_count);

#endif
