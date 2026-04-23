#include "SceneParticleVisuals.h"

#include "SceneInternal.h"
#include "ScenePhysicsInternal.h"
#include "SceneParticleVisualsInternal.h"
#include "../Physics/PhysicsParticles.h"

#include "../Core/Memory.h"

#include <stdlib.h>
#include <string.h>

#define SCENE_PARTICLE_DEFAULT_BLUE_ARGB 0xd02f8dffU
#define SCENE_PARTICLE_DEFAULT_MESH_ARGB 0xccfff000U
#define SCENE_PARTICLE_DEFAULT_MESH_LAYER 1000

static bool SceneParticleVisuals_HandlesEqual(
    PhysicsParticleSystemHandle a,
    PhysicsParticleSystemHandle b
)
{
    return a.index1 == b.index1 &&
           a.world0 == b.world0 &&
           a.generation == b.generation;
}

static int SceneParticleVisuals_FindIndex(
    const SceneParticleVisualState* state,
    PhysicsParticleSystemHandle particle_system
)
{
    size_t index = 0U;

    if (state == NULL || particle_system.index1 == 0)
    {
        return -1;
    }

    for (index = 0U; index < state->binding_count; ++index)
    {
        if (SceneParticleVisuals_HandlesEqual(state->bindings[index].particle_system, particle_system))
        {
            return (int)index;
        }
    }
    return -1;
}

static bool SceneParticleVisuals_Reserve(SceneParticleVisualState* state, size_t required_capacity)
{
    SceneParticleVisualDesc* resized = NULL;
    size_t next_capacity = 0U;

    if (state == NULL)
    {
        return false;
    }
    if (state->binding_capacity >= required_capacity)
    {
        return true;
    }

    next_capacity = state->binding_capacity > 0U ? state->binding_capacity : 2U;
    while (next_capacity < required_capacity)
    {
        next_capacity *= 2U;
    }

    resized = (SceneParticleVisualDesc*)realloc(
        state->bindings,
        next_capacity * sizeof(*state->bindings)
    );
    if (resized == NULL)
    {
        return false;
    }

    state->bindings = resized;
    state->binding_capacity = next_capacity;
    return true;
}

bool Scene_RegisterParticleVisual(Scene* scene, const SceneParticleVisualDesc* desc)
{
    SceneParticleVisualDesc binding;
    int index = -1;

    if (scene == NULL ||
        scene->particle_visuals == NULL ||
        desc == NULL ||
        desc->particle_system.index1 == 0)
    {
        return false;
    }
    if (scene->physics == NULL ||
        scene->physics->physics_world == NULL ||
        !PhysicsWorld_IsParticleSystemValid(scene->physics->physics_world, desc->particle_system))
    {
        return false;
    }

    binding = *desc;
    if (binding.particles_visible && binding.particle_material_handle.id == 0U)
    {
        binding.particle_material_handle = scene->particle_visuals->default_resources.particle_material_handle;
    }
    if (binding.mesh_visible)
    {
        if (binding.mesh_handle.id == 0U)
        {
            binding.mesh_handle = scene->particle_visuals->default_resources.mesh_handle;
        }
        if (binding.mesh_material_handle.id == 0U)
        {
            binding.mesh_material_handle = scene->particle_visuals->default_resources.mesh_material_handle;
        }
    }
    if ((!binding.particles_visible && !binding.mesh_visible) ||
        (binding.particles_visible && binding.particle_material_handle.id == 0U) ||
        (binding.mesh_visible &&
         (binding.mesh_handle.id == 0U || binding.mesh_material_handle.id == 0U)))
    {
        return false;
    }

    binding.particle_fallback_tint = desc->particle_fallback_tint.value != 0U
        ? desc->particle_fallback_tint
        : (Color32){ SCENE_PARTICLE_DEFAULT_BLUE_ARGB };
    binding.mesh_tint = desc->mesh_tint.value != 0U
        ? desc->mesh_tint
        : (Color32){ SCENE_PARTICLE_DEFAULT_MESH_ARGB };
    if (binding.mesh_visible && desc->mesh_layer == 0)
    {
        binding.mesh_layer = SCENE_PARTICLE_DEFAULT_MESH_LAYER;
    }
    index = SceneParticleVisuals_FindIndex(scene->particle_visuals, desc->particle_system);
    if (index >= 0)
    {
        scene->particle_visuals->bindings[index] = binding;
        return true;
    }

    if (!SceneParticleVisuals_Reserve(scene->particle_visuals, scene->particle_visuals->binding_count + 1U))
    {
        return false;
    }

    scene->particle_visuals->bindings[scene->particle_visuals->binding_count++] = binding;
    return true;
}

bool Scene_SetDefaultParticleVisualResources(
    Scene* scene,
    const ParticleVisualResourceHandles* handles
)
{
    if (scene == NULL || scene->particle_visuals == NULL || handles == NULL)
    {
        return false;
    }

    scene->particle_visuals->default_resources = *handles;
    return true;
}

bool Scene_UnregisterParticleVisual(Scene* scene, PhysicsParticleSystemHandle particle_system)
{
    int index = -1;

    if (scene == NULL || scene->particle_visuals == NULL)
    {
        return false;
    }

    index = SceneParticleVisuals_FindIndex(scene->particle_visuals, particle_system);
    if (index < 0)
    {
        return false;
    }

    memmove(
        &scene->particle_visuals->bindings[index],
        &scene->particle_visuals->bindings[index + 1],
        (scene->particle_visuals->binding_count - (size_t)index - 1U) *
            sizeof(*scene->particle_visuals->bindings)
    );
    scene->particle_visuals->binding_count -= 1U;
    memset(
        &scene->particle_visuals->bindings[scene->particle_visuals->binding_count],
        0,
        sizeof(*scene->particle_visuals->bindings)
    );
    return true;
}

size_t Scene_GetParticleVisualCount(const Scene* scene)
{
    return scene != NULL && scene->particle_visuals != NULL
        ? scene->particle_visuals->binding_count
        : 0U;
}

const SceneParticleVisualDesc* Scene_GetParticleVisualBindings(const Scene* scene, size_t* out_count)
{
    if (out_count != NULL)
    {
        *out_count = 0U;
    }
    if (scene == NULL || scene->particle_visuals == NULL)
    {
        return NULL;
    }
    if (out_count != NULL)
    {
        *out_count = scene->particle_visuals->binding_count;
    }
    return scene->particle_visuals->bindings;
}

void SceneParticleVisualState_Dispose(SceneParticleVisualState* state)
{
    if (state == NULL)
    {
        return;
    }

    free(state->bindings);
    memset(state, 0, sizeof(*state));
}
