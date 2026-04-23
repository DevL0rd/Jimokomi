#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_PARTICLE_VISUALS_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_PARTICLE_VISUALS_H

#include "Scene.h"
#include "../Physics/PhysicsHandles.h"
#include "../Rendering/ParticleVisualResources.h"
#include "../Rendering/RenderCommon.h"
#include "../Rendering/ResourceTypes.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct SceneParticleVisualDesc
{
    PhysicsParticleSystemHandle particle_system;
    ResourceHandle mesh_handle;
    ResourceHandle particle_material_handle;
    ResourceHandle mesh_material_handle;
    Color32 particle_fallback_tint;
    int particle_layer;
    bool particles_visible;
    bool mesh_visible;
    Color32 mesh_tint;
    int mesh_layer;
    float mesh_cell_size;
    float mesh_influence_radius;
    float mesh_threshold;
} SceneParticleVisualDesc;

bool Scene_SetDefaultParticleVisualResources(
    Scene* scene,
    const ParticleVisualResourceHandles* handles
);
bool Scene_RegisterParticleVisual(Scene* scene, const SceneParticleVisualDesc* desc);
bool Scene_UnregisterParticleVisual(Scene* scene, PhysicsParticleSystemHandle particle_system);
size_t Scene_GetParticleVisualCount(const Scene* scene);

#endif
