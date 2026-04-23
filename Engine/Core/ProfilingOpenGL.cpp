#include "Profiling.h"

#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)

#include <external/glad.h>
#define Texture RaylibNativeTexture
#define RenderTexture RaylibNativeRenderTexture
#include <raylib.h>
#undef RenderTexture
#undef Texture
#include <rlgl.h>
#include <tracy/TracyOpenGL.hpp>

#include <cstring>
#include <new>

namespace
{
    bool g_engine_profile_gpu_initialized = false;

    bool engine_profile_gpu_has_timer_queries()
    {
        return glad_glGenQueries != nullptr &&
               glad_glQueryCounter != nullptr &&
               glad_glGetInteger64v != nullptr &&
               glad_glGetQueryiv != nullptr &&
               glad_glGetQueryObjectiv != nullptr &&
               glad_glGetQueryObjectui64v != nullptr;
    }
}

static_assert(sizeof(EngineProfileGpuZone) >= sizeof(tracy::GpuCtxScope), "EngineProfileGpuZone storage too small");

extern "C" void engine_profile_gpu_init(void)
{
    if (g_engine_profile_gpu_initialized)
    {
        return;
    }

    if (!engine_profile_gpu_has_timer_queries())
    {
        return;
    }

    TracyGpuContext;
    TracyGpuContextName("Jimokomi OpenGL", 15);
    g_engine_profile_gpu_initialized = true;
}

extern "C" void engine_profile_gpu_shutdown(void)
{
    if (!g_engine_profile_gpu_initialized)
    {
        return;
    }

    tracy::GetGpuCtx().ptr->~GpuCtx();
    tracy::tracy_free(tracy::GetGpuCtx().ptr);
    tracy::GetGpuCtx().ptr = nullptr;
    g_engine_profile_gpu_initialized = false;
}

extern "C" bool engine_profile_gpu_is_available(void)
{
    return g_engine_profile_gpu_initialized;
}

extern "C" void engine_profile_gpu_collect(void)
{
    if (!g_engine_profile_gpu_initialized)
    {
        return;
    }

    TracyGpuCollect;
}

extern "C" void engine_profile_gpu_zone_begin(EngineProfileGpuZone* zone, const char* name)
{
    if (zone == nullptr)
    {
        return;
    }

    memset(zone, 0, sizeof(*zone));
    if (!g_engine_profile_gpu_initialized || name == nullptr || name[0] == '\0')
    {
        return;
    }

    new (zone->storage) tracy::GpuCtxScope(
        TracyLine,
        TracyFile,
        strlen(TracyFile),
        TracyFunction,
        strlen(TracyFunction),
        name,
        strlen(name),
        true
    );
    zone->active = 1;
}

extern "C" void engine_profile_gpu_zone_begin_callstack(EngineProfileGpuZone* zone, const char* name, int depth)
{
    if (zone == nullptr)
    {
        return;
    }

    memset(zone, 0, sizeof(*zone));
    if (!g_engine_profile_gpu_initialized || name == nullptr || name[0] == '\0')
    {
        return;
    }

    new (zone->storage) tracy::GpuCtxScope(
        TracyLine,
        TracyFile,
        strlen(TracyFile),
        TracyFunction,
        strlen(TracyFunction),
        name,
        strlen(name),
        depth,
        true
    );
    zone->active = 1;
}

extern "C" void engine_profile_gpu_zone_end(EngineProfileGpuZone* zone)
{
    if (zone == nullptr || zone->active == 0)
    {
        return;
    }

    auto* scope = reinterpret_cast<tracy::GpuCtxScope*>(static_cast<void*>(zone->storage));
    scope->~GpuCtxScope();
    zone->active = 0;
}

#else

extern "C" void engine_profile_gpu_init(void) {}
extern "C" void engine_profile_gpu_shutdown(void) {}
extern "C" bool engine_profile_gpu_is_available(void) { return false; }
extern "C" void engine_profile_gpu_collect(void) {}
extern "C" void engine_profile_gpu_zone_begin(EngineProfileGpuZone* zone, const char* name)
{
    (void)zone;
    (void)name;
}
extern "C" void engine_profile_gpu_zone_begin_callstack(EngineProfileGpuZone* zone, const char* name, int depth)
{
    (void)zone;
    (void)name;
    (void)depth;
}
extern "C" void engine_profile_gpu_zone_end(EngineProfileGpuZone* zone)
{
    (void)zone;
}

#endif
