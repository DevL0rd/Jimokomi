#ifndef JIMOKOMI_ENGINE_RENDERING_PARTICLE_VISUAL_RESOURCES_H
#define JIMOKOMI_ENGINE_RENDERING_PARTICLE_VISUAL_RESOURCES_H

#include "ResourceTypes.h"

#include <stdbool.h>

typedef struct Renderer Renderer;

typedef struct ParticleVisualResourceHandles
{
    ResourceHandle mesh_handle;
    ResourceHandle particle_material_handle;
    ResourceHandle mesh_material_handle;
} ParticleVisualResourceHandles;

bool renderer_register_default_particle_visual_resources(
    Renderer* renderer,
    ParticleVisualResourceHandles* handles
);

#endif
