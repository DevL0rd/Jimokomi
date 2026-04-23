#ifndef JIMOKOMI_ENGINE_CORE_PROFILING_H
#define JIMOKOMI_ENGINE_CORE_PROFILING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum EngineProfilePlotFormat
{
    ENGINE_PROFILE_PLOT_NUMBER = 0,
    ENGINE_PROFILE_PLOT_MEMORY = 1,
    ENGINE_PROFILE_PLOT_PERCENTAGE = 2,
    ENGINE_PROFILE_PLOT_WATT = 3
} EngineProfilePlotFormat;

typedef struct EngineProfileGpuZone
{
    uint64_t storage[8];
    int active;
} EngineProfileGpuZone;

#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
#include <tracy/TracyC.h>
typedef TracyCLockCtx EngineProfileLockCtx;
#else
typedef void* EngineProfileLockCtx;
#endif

void engine_profile_set_thread_name(const char* name);
void engine_profile_plot_value(const char* name, double value);
void engine_profile_plot_integer(const char* name, int64_t value);
void engine_profile_plot_config(const char* name, EngineProfilePlotFormat format, bool step, bool fill, uint32_t color);
void engine_profile_message(const char* text);
void engine_profile_message_color(const char* text, uint32_t color);
void engine_profile_app_info(const char* text);
bool engine_profile_is_connected(void);
void engine_profile_frame_mark_named(const char* name);
void engine_profile_frame_mark_start(const char* name);
void engine_profile_frame_mark_end(const char* name);
void engine_profile_frame_image(const void* image, uint16_t width, uint16_t height, uint8_t offset, bool flip);
void engine_profile_memory_alloc(const void* memory, size_t size, const char* name, int callstack_depth);
void engine_profile_memory_free(const void* memory, const char* name, int callstack_depth);
EngineProfileLockCtx engine_profile_lock_create(const char* name);
void engine_profile_lock_destroy(EngineProfileLockCtx lock);
void engine_profile_lock_before_lock(EngineProfileLockCtx lock);
void engine_profile_lock_after_lock(EngineProfileLockCtx lock);
void engine_profile_lock_after_unlock(EngineProfileLockCtx lock);
void engine_profile_lock_after_try_lock(EngineProfileLockCtx lock, bool acquired);
void engine_profile_lock_set_name(EngineProfileLockCtx lock, const char* name);
void engine_profile_gpu_init(void);
void engine_profile_gpu_shutdown(void);
bool engine_profile_gpu_is_available(void);
void engine_profile_gpu_collect(void);
void engine_profile_gpu_zone_begin(EngineProfileGpuZone* zone, const char* name);
void engine_profile_gpu_zone_begin_callstack(EngineProfileGpuZone* zone, const char* name, int depth);
void engine_profile_gpu_zone_end(EngineProfileGpuZone* zone);

#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)

#define ENGINE_PROFILE_ZONE_BEGIN(ctx, name) TracyCZoneN(ctx, name, true)
#define ENGINE_PROFILE_ZONE_BEGIN_COLOR(ctx, name, color) TracyCZoneNC(ctx, name, color, true)
#define ENGINE_PROFILE_ZONE_BEGIN_CALLSTACK(ctx, name, depth) TracyCZoneNS(ctx, name, depth, true)
#define ENGINE_PROFILE_ZONE_BEGIN_COLOR_CALLSTACK(ctx, name, color, depth) TracyCZoneNCS(ctx, name, color, depth, true)
#define ENGINE_PROFILE_ZONE_END(ctx) TracyCZoneEnd(ctx)
#define ENGINE_PROFILE_ZONE_TEXT(ctx, text, size) TracyCZoneText(ctx, text, size)
#define ENGINE_PROFILE_ZONE_NAME(ctx, text, size) TracyCZoneName(ctx, text, size)
#define ENGINE_PROFILE_ZONE_COLOR(ctx, color) TracyCZoneColor(ctx, color)
#define ENGINE_PROFILE_ZONE_VALUE(ctx, value) TracyCZoneValue(ctx, value)
#define ENGINE_PROFILE_FRAME_MARK() TracyCFrameMark

#else

#define ENGINE_PROFILE_ZONE_BEGIN(ctx, name)
#define ENGINE_PROFILE_ZONE_BEGIN_COLOR(ctx, name, color)
#define ENGINE_PROFILE_ZONE_BEGIN_CALLSTACK(ctx, name, depth)
#define ENGINE_PROFILE_ZONE_BEGIN_COLOR_CALLSTACK(ctx, name, color, depth)
#define ENGINE_PROFILE_ZONE_END(ctx)
#define ENGINE_PROFILE_ZONE_TEXT(ctx, text, size)
#define ENGINE_PROFILE_ZONE_NAME(ctx, text, size)
#define ENGINE_PROFILE_ZONE_COLOR(ctx, color)
#define ENGINE_PROFILE_ZONE_VALUE(ctx, value)
#define ENGINE_PROFILE_FRAME_MARK()

#endif

#if defined(__cplusplus)
}
#endif

#endif
