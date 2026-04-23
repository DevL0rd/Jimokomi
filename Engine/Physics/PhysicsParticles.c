#include "PhysicsParticles.h"

#include "PhysicsWorldLifecycleInternal.h"
#include "PhysicsWorldParticlesInternal.h"

#include <corephys/corephys.h>

#include <stdlib.h>
#include <string.h>

static b2ParticleSystemId PhysicsWorld_LoadParticleSystemHandle(PhysicsParticleSystemHandle handle)
{
    return (b2ParticleSystemId){ handle.index1, handle.world0, handle.generation };
}

static PhysicsParticleSystemHandle PhysicsWorld_StoreParticleSystemHandle(b2ParticleSystemId system_id)
{
    return (PhysicsParticleSystemHandle){ system_id.index1, system_id.world0, system_id.generation };
}

static bool PhysicsWorld_ParticleSystemHandlesEqual(
    PhysicsParticleSystemHandle a,
    PhysicsParticleSystemHandle b
)
{
    return a.index1 == b.index1 &&
           a.world0 == b.world0 &&
           a.generation == b.generation;
}

static int PhysicsWorld_FindParticleSystemIndex(
    const PhysicsWorld* world,
    PhysicsParticleSystemHandle handle
)
{
    size_t index = 0U;

    if (world == NULL || world->particles == NULL || handle.index1 == 0)
    {
        return -1;
    }

    for (index = 0U; index < world->particles->system_count; ++index)
    {
        if (PhysicsWorld_ParticleSystemHandlesEqual(world->particles->systems[index], handle))
        {
            return (int)index;
        }
    }

    return -1;
}

static bool PhysicsWorld_ReserveParticleSystems(PhysicsWorldParticleState* particles, size_t required_capacity)
{
    PhysicsParticleSystemHandle* resized = NULL;
    size_t next_capacity = 0U;

    if (particles == NULL)
    {
        return false;
    }
    if (particles->system_capacity >= required_capacity)
    {
        return true;
    }

    next_capacity = particles->system_capacity > 0U ? particles->system_capacity : 2U;
    while (next_capacity < required_capacity)
    {
        next_capacity *= 2U;
    }

    resized = (PhysicsParticleSystemHandle*)realloc(
        particles->systems,
        next_capacity * sizeof(*particles->systems)
    );
    if (resized == NULL)
    {
        return false;
    }

    particles->systems = resized;
    particles->system_capacity = next_capacity;
    return true;
}

static bool PhysicsWorld_TrackParticleSystem(
    PhysicsWorld* world,
    PhysicsParticleSystemHandle handle
)
{
    if (world == NULL || world->particles == NULL)
    {
        return false;
    }
    if (!PhysicsWorld_ReserveParticleSystems(world->particles, world->particles->system_count + 1U))
    {
        return false;
    }

    world->particles->systems[world->particles->system_count++] = handle;
    return true;
}

static void PhysicsWorld_UntrackParticleSystem(
    PhysicsWorld* world,
    PhysicsParticleSystemHandle handle
)
{
    int index = PhysicsWorld_FindParticleSystemIndex(world, handle);

    if (world == NULL || world->particles == NULL || index < 0)
    {
        return;
    }

    memmove(
        &world->particles->systems[index],
        &world->particles->systems[index + 1],
        (world->particles->system_count - (size_t)index - 1U) * sizeof(*world->particles->systems)
    );
    world->particles->system_count -= 1U;
    memset(&world->particles->systems[world->particles->system_count], 0, sizeof(*world->particles->systems));
}

static b2ParticleColor PhysicsWorld_UnpackParticleColor(uint32_t color_argb)
{
    b2ParticleColor color;

    color.r = (uint8_t)((color_argb >> 16U) & 0xffU);
    color.g = (uint8_t)((color_argb >> 8U) & 0xffU);
    color.b = (uint8_t)(color_argb & 0xffU);
    color.a = (uint8_t)((color_argb >> 24U) & 0xffU);
    if (color.a == 0U && color_argb != 0U)
    {
        color.a = 255U;
    }
    return color;
}

static uint32_t PhysicsWorld_PackParticleColor(b2ParticleColor color)
{
    return ((uint32_t)color.a << 24U) |
           ((uint32_t)color.r << 16U) |
           ((uint32_t)color.g << 8U) |
           (uint32_t)color.b;
}

typedef struct PhysicsParticleRenderQueryContext
{
    PhysicsParticleRenderData* particles;
    size_t capacity;
    size_t count;
    float radius;
} PhysicsParticleRenderQueryContext;

static bool PhysicsWorld_CopyQueriedParticleRenderData(
    b2ParticleSystemId system_id,
    int particle_index,
    b2Vec2 position,
    b2ParticleColor color,
    void* user_data,
    void* context
)
{
    PhysicsParticleRenderQueryContext* query = (PhysicsParticleRenderQueryContext*)context;
    PhysicsParticleRenderData* particle = NULL;

    (void)system_id;
    (void)particle_index;

    if (query == NULL || query->particles == NULL || query->count >= query->capacity)
    {
        return false;
    }

    particle = &query->particles[query->count++];
    particle->position = (Vec2){ position.x, position.y };
    particle->radius = query->radius;
    particle->color_argb = PhysicsWorld_PackParticleColor(color);
    particle->user_tag = (uintptr_t)user_data;
    return query->count < query->capacity;
}

bool PhysicsWorld_CreateParticleSystem(
    PhysicsWorld* world,
    const PhysicsParticleSystemDesc* desc,
    PhysicsParticleSystemHandle* out_handle
)
{
    b2ParticleSystemDef system_def;
    b2ParticleSystemId system_id;
    PhysicsParticleSystemHandle handle;

    if (out_handle != NULL)
    {
        memset(out_handle, 0, sizeof(*out_handle));
    }
    if (world == NULL || world->lifecycle == NULL || world->particles == NULL || !world->lifecycle->has_world)
    {
        return false;
    }

    system_def = b2DefaultParticleSystemDef();
    if (desc != NULL)
    {
        if (desc->radius > 0.0f)
        {
            system_def.radius = desc->radius;
        }
        if (desc->density > 0.0f)
        {
            system_def.density = desc->density;
        }
        if (desc->gravity_scale > 0.0f)
        {
            system_def.gravityScale = desc->gravity_scale;
        }
        if (desc->damping_strength > 0.0f)
        {
            system_def.dampingStrength = desc->damping_strength;
        }
        if (desc->pressure_strength > 0.0f)
        {
            system_def.pressureStrength = desc->pressure_strength;
        }
        if (desc->viscous_strength > 0.0f)
        {
            system_def.viscousStrength = desc->viscous_strength;
        }
        if (desc->surface_tension_pressure_strength > 0.0f)
        {
            system_def.surfaceTensionPressureStrength = desc->surface_tension_pressure_strength;
        }
        if (desc->surface_tension_normal_strength > 0.0f)
        {
            system_def.surfaceTensionNormalStrength = desc->surface_tension_normal_strength;
        }
        if (desc->max_count > 0)
        {
            system_def.maxCount = desc->max_count;
        }
        if (desc->iteration_count > 0)
        {
            system_def.iterationCount = desc->iteration_count;
        }
        system_def.destroyByAge = desc->destroy_by_age;
        if (desc->lifetime_granularity > 0.0f)
        {
            system_def.lifetimeGranularity = desc->lifetime_granularity;
        }
    }

    system_id = b2CreateParticleSystem(world->lifecycle->world_id, &system_def);
    if (!b2ParticleSystem_IsValid(system_id))
    {
        return false;
    }

    handle = PhysicsWorld_StoreParticleSystemHandle(system_id);
    if (!PhysicsWorld_TrackParticleSystem(world, handle))
    {
        b2DestroyParticleSystem(system_id);
        return false;
    }

    if (out_handle != NULL)
    {
        *out_handle = handle;
    }
    return true;
}

void PhysicsWorld_DestroyParticleSystem(PhysicsWorld* world, PhysicsParticleSystemHandle handle)
{
    b2ParticleSystemId system_id;

    if (!PhysicsWorld_IsParticleSystemValid(world, handle))
    {
        return;
    }

    system_id = PhysicsWorld_LoadParticleSystemHandle(handle);
    b2DestroyParticleSystem(system_id);
    PhysicsWorld_UntrackParticleSystem(world, handle);
}

bool PhysicsWorld_IsParticleSystemValid(const PhysicsWorld* world, PhysicsParticleSystemHandle handle)
{
    return world != NULL &&
           world->lifecycle != NULL &&
           world->particles != NULL &&
           world->lifecycle->has_world &&
           PhysicsWorld_FindParticleSystemIndex(world, handle) >= 0 &&
           b2ParticleSystem_IsValid(PhysicsWorld_LoadParticleSystemHandle(handle));
}

bool PhysicsWorld_CreateParticle(
    PhysicsWorld* world,
    PhysicsParticleSystemHandle handle,
    const PhysicsParticleDesc* desc,
    int* out_particle_index
)
{
    b2ParticleDef particle_def;
    int particle_index = -1;

    if (out_particle_index != NULL)
    {
        *out_particle_index = -1;
    }
    if (desc == NULL || !PhysicsWorld_IsParticleSystemValid(world, handle))
    {
        return false;
    }

    particle_def = b2DefaultParticleDef();
    particle_def.flags = desc->flags;
    particle_def.position = (b2Vec2){ desc->position.x, desc->position.y };
    particle_def.velocity = (b2Vec2){ desc->velocity.x, desc->velocity.y };
    particle_def.color = PhysicsWorld_UnpackParticleColor(desc->color_argb);
    particle_def.lifetime = desc->lifetime_seconds;
    particle_def.userData = (void*)desc->user_tag;

    particle_index = b2ParticleSystem_CreateParticle(PhysicsWorld_LoadParticleSystemHandle(handle), &particle_def);
    if (particle_index < 0)
    {
        return false;
    }

    if (out_particle_index != NULL)
    {
        *out_particle_index = particle_index;
    }
    return true;
}

size_t PhysicsWorld_GetParticleSystemParticleCount(
    const PhysicsWorld* world,
    PhysicsParticleSystemHandle handle
)
{
    int particle_count = 0;

    if (!PhysicsWorld_IsParticleSystemValid(world, handle))
    {
        return 0U;
    }

    particle_count = b2ParticleSystem_GetParticleCount(PhysicsWorld_LoadParticleSystemHandle(handle));
    return particle_count > 0 ? (size_t)particle_count : 0U;
}

float PhysicsWorld_GetParticleSystemRadius(const PhysicsWorld* world, PhysicsParticleSystemHandle handle)
{
    if (!PhysicsWorld_IsParticleSystemValid(world, handle))
    {
        return 0.0f;
    }

    return b2ParticleSystem_GetRadius(PhysicsWorld_LoadParticleSystemHandle(handle));
}

size_t PhysicsWorld_GetParticleCount(const PhysicsWorld* world)
{
    size_t total_count = 0U;
    size_t system_index = 0U;

    if (world == NULL || world->particles == NULL)
    {
        return 0U;
    }

    for (system_index = 0U; system_index < world->particles->system_count; ++system_index)
    {
        total_count += PhysicsWorld_GetParticleSystemParticleCount(world, world->particles->systems[system_index]);
    }
    return total_count;
}

size_t PhysicsWorld_CopyParticleSystemRenderData(
    const PhysicsWorld* world,
    PhysicsParticleSystemHandle handle,
    PhysicsParticleRenderData* particles,
    size_t capacity
)
{
    b2ParticleSystemId system_id;
    const b2Vec2* positions = NULL;
    const b2ParticleColor* colors = NULL;
    void* const* user_data = NULL;
    float radius = 0.0f;
    int particle_count = 0;
    int particle_index = 0;
    size_t copied_count = 0U;

    if (!PhysicsWorld_IsParticleSystemValid(world, handle) || particles == NULL || capacity == 0U)
    {
        return 0U;
    }

    system_id = PhysicsWorld_LoadParticleSystemHandle(handle);
    particle_count = b2ParticleSystem_GetParticleCount(system_id);
    positions = b2ParticleSystem_GetPositionBuffer(system_id);
    colors = b2ParticleSystem_GetColorBuffer(system_id);
    user_data = b2ParticleSystem_GetUserDataBuffer(system_id);
    radius = b2ParticleSystem_GetRadius(system_id);
    if (particle_count <= 0 || positions == NULL)
    {
        return 0U;
    }

    for (particle_index = 0; particle_index < particle_count && copied_count < capacity; ++particle_index)
    {
        PhysicsParticleRenderData* particle = &particles[copied_count++];

        particle->position = (Vec2){ positions[particle_index].x, positions[particle_index].y };
        particle->radius = radius;
        particle->color_argb = colors != NULL
            ? PhysicsWorld_PackParticleColor(colors[particle_index])
            : 0xffffffffU;
        particle->user_tag = user_data != NULL ? (uintptr_t)user_data[particle_index] : 0U;
    }

    return copied_count;
}

size_t PhysicsWorld_CopyParticleSystemRenderDataInAabb(
    const PhysicsWorld* world,
    PhysicsParticleSystemHandle handle,
    Aabb bounds,
    PhysicsParticleRenderData* particles,
    size_t capacity
)
{
    b2ParticleSystemId system_id;
    b2AABB query_bounds;
    PhysicsParticleRenderQueryContext query_context;

    if (!PhysicsWorld_IsParticleSystemValid(world, handle) || particles == NULL || capacity == 0U)
    {
        return 0U;
    }

    system_id = PhysicsWorld_LoadParticleSystemHandle(handle);
    query_bounds = (b2AABB){
        .lowerBound = { bounds.min_x, bounds.min_y },
        .upperBound = { bounds.max_x, bounds.max_y }
    };
    query_context = (PhysicsParticleRenderQueryContext){
        .particles = particles,
        .capacity = capacity,
        .count = 0U,
        .radius = b2ParticleSystem_GetRadius(system_id)
    };

    (void)b2ParticleSystem_QueryParticlesInAABB(
        system_id,
        query_bounds,
        PhysicsWorld_CopyQueriedParticleRenderData,
        &query_context
    );
    return query_context.count;
}

size_t PhysicsWorld_CopyParticleRenderData(
    const PhysicsWorld* world,
    PhysicsParticleRenderData* particles,
    size_t capacity
)
{
    size_t copied_count = 0U;
    size_t system_index = 0U;

    if (world == NULL || world->particles == NULL || particles == NULL || capacity == 0U)
    {
        return 0U;
    }

    for (system_index = 0U; system_index < world->particles->system_count; ++system_index)
    {
        PhysicsParticleSystemHandle handle = world->particles->systems[system_index];
        copied_count += PhysicsWorld_CopyParticleSystemRenderData(
            world,
            handle,
            particles + copied_count,
            capacity - copied_count
        );
    }

    return copied_count;
}

void PhysicsWorld_DestroyParticleState(PhysicsWorld* world)
{
    size_t index = 0U;

    if (world == NULL || world->particles == NULL)
    {
        return;
    }

    for (index = 0U; index < world->particles->system_count; ++index)
    {
        b2ParticleSystemId system_id = PhysicsWorld_LoadParticleSystemHandle(world->particles->systems[index]);
        if (b2ParticleSystem_IsValid(system_id))
        {
            b2DestroyParticleSystem(system_id);
        }
    }

    free(world->particles->systems);
    free(world->particles);
    world->particles = NULL;
}
