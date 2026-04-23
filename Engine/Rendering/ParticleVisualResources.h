#ifndef JIMOKOMI_ENGINE_RENDERING_PARTICLE_VISUAL_RESOURCES_H
#define JIMOKOMI_ENGINE_RENDERING_PARTICLE_VISUAL_RESOURCES_H

#include "RenderCommon.h"
#include "RenderTypes.h"
#include "ResourceTypes.h"
#include "../Physics/PhysicsParticles.h"
#include "../Core/TaskSystem.h"

#include <stdbool.h>

typedef struct Renderer Renderer;

typedef struct ParticleVisualResourceHandles
{
    ResourceHandle mesh_handle;
    ResourceHandle particle_material_handle;
    ResourceHandle mesh_material_handle;
} ParticleVisualResourceHandles;

typedef struct ParticleSurfaceMeshBuildInput
{
    const TaskSystem* task_system;
    const PhysicsParticleRenderData* particles;
    size_t particle_count;
    Color32 fallback_tint;
    Color32 mesh_tint;
    int mesh_layer;
    float cell_size;
    float influence_radius;
    float threshold;
} ParticleSurfaceMeshBuildInput;

bool particle_surface_mesh_build_geometry(
    ProceduralMeshWriter* writer,
    const ProceduralMeshContext* context,
    const ParticleSurfaceMeshBuildInput* input
);

bool renderer_register_default_particle_visual_resources(
    Renderer* renderer,
    ParticleVisualResourceHandles* handles
);

#endif
