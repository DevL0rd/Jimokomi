#include "Profiling.h"

#include "PlatformRuntimeInternal.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct EngineProfileMemoryTracker
{
    const void** items;
    size_t count;
    size_t capacity;
} EngineProfileMemoryTracker;

static EngineProfileMemoryTracker g_engine_profile_memory_tracker;

#if defined(_WIN32)
static INIT_ONCE g_engine_profile_memory_tracker_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION g_engine_profile_memory_tracker_mutex;

static BOOL CALLBACK engine_profile_memory_tracker_init_once(PINIT_ONCE init_once, PVOID parameter, PVOID* context)
{
    (void)init_once;
    (void)parameter;
    (void)context;
    InitializeCriticalSection(&g_engine_profile_memory_tracker_mutex);
    return TRUE;
}

static void engine_profile_memory_tracker_lock(void)
{
    InitOnceExecuteOnce(&g_engine_profile_memory_tracker_once, engine_profile_memory_tracker_init_once, NULL, NULL);
    EnterCriticalSection(&g_engine_profile_memory_tracker_mutex);
}

static void engine_profile_memory_tracker_unlock(void)
{
    LeaveCriticalSection(&g_engine_profile_memory_tracker_mutex);
}
#else
static pthread_mutex_t g_engine_profile_memory_tracker_mutex = PTHREAD_MUTEX_INITIALIZER;

static void engine_profile_memory_tracker_lock(void)
{
    pthread_mutex_lock(&g_engine_profile_memory_tracker_mutex);
}

static void engine_profile_memory_tracker_unlock(void)
{
    pthread_mutex_unlock(&g_engine_profile_memory_tracker_mutex);
}
#endif

static bool engine_profile_memory_tracker_add(const void* memory)
{
    size_t index = 0U;

    if (memory == NULL)
    {
        return false;
    }

    engine_profile_memory_tracker_lock();
    for (index = 0U; index < g_engine_profile_memory_tracker.count; ++index)
    {
        if (g_engine_profile_memory_tracker.items[index] == memory)
        {
            engine_profile_memory_tracker_unlock();
            return false;
        }
    }

    if (g_engine_profile_memory_tracker.count == g_engine_profile_memory_tracker.capacity)
    {
        size_t next_capacity = g_engine_profile_memory_tracker.capacity > 0U ? g_engine_profile_memory_tracker.capacity * 2U : 256U;
        const void** next_items = (const void**)realloc(
            g_engine_profile_memory_tracker.items, next_capacity * sizeof(*next_items));
        if (next_items == NULL)
        {
            engine_profile_memory_tracker_unlock();
            return false;
        }

        g_engine_profile_memory_tracker.items = next_items;
        g_engine_profile_memory_tracker.capacity = next_capacity;
    }

    g_engine_profile_memory_tracker.items[g_engine_profile_memory_tracker.count++] = memory;
    engine_profile_memory_tracker_unlock();
    return true;
}

static bool engine_profile_memory_tracker_remove(const void* memory)
{
    size_t index = 0U;

    if (memory == NULL)
    {
        return false;
    }

    engine_profile_memory_tracker_lock();
    for (index = 0U; index < g_engine_profile_memory_tracker.count; ++index)
    {
        if (g_engine_profile_memory_tracker.items[index] == memory)
        {
            g_engine_profile_memory_tracker.items[index] =
                g_engine_profile_memory_tracker.items[g_engine_profile_memory_tracker.count - 1U];
            g_engine_profile_memory_tracker.count -= 1U;
            engine_profile_memory_tracker_unlock();
            return true;
        }
    }

    engine_profile_memory_tracker_unlock();
    return false;
}
#endif

void engine_profile_set_thread_name(const char* name)
{
    if (name == NULL || name[0] == '\0')
    {
        return;
    }

    engine_platform_set_current_thread_name(name);
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    TracyCSetThreadName(name);
#endif
}

void engine_profile_plot_value(const char* name, double value)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (name != NULL)
    {
        TracyCPlot(name, value);
    }
#else
    (void)name;
    (void)value;
#endif
}

void engine_profile_plot_integer(const char* name, int64_t value)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (name != NULL)
    {
        TracyCPlotI(name, value);
    }
#else
    (void)name;
    (void)value;
#endif
}

void engine_profile_plot_config(const char* name, EngineProfilePlotFormat format, bool step, bool fill, uint32_t color)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (name != NULL)
    {
        TracyCPlotConfig(name, (int)format, step ? 1 : 0, fill ? 1 : 0, color);
    }
#else
    (void)name;
    (void)format;
    (void)step;
    (void)fill;
    (void)color;
#endif
}

void engine_profile_message(const char* text)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (text != NULL)
    {
        TracyCMessage(text, 0);
    }
#else
    (void)text;
#endif
}

void engine_profile_message_color(const char* text, uint32_t color)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (text != NULL)
    {
        TracyCMessageLC(text, color);
    }
#else
    (void)text;
    (void)color;
#endif
}

void engine_profile_app_info(const char* text)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (text != NULL)
    {
        TracyCAppInfo(text, strlen(text));
    }
#else
    (void)text;
#endif
}

bool engine_profile_is_connected(void)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    return TracyCIsConnected != 0;
#else
    return false;
#endif
}

void engine_profile_frame_mark_named(const char* name)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    TracyCFrameMarkNamed(name);
#else
    (void)name;
#endif
}

void engine_profile_frame_mark_start(const char* name)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    TracyCFrameMarkStart(name);
#else
    (void)name;
#endif
}

void engine_profile_frame_mark_end(const char* name)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    TracyCFrameMarkEnd(name);
#else
    (void)name;
#endif
}

void engine_profile_frame_image(const void* image, uint16_t width, uint16_t height, uint8_t offset, bool flip)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (image != NULL && width > 0U && height > 0U)
    {
        TracyCFrameImage(image, width, height, offset, flip ? 1 : 0);
    }
#else
    (void)image;
    (void)width;
    (void)height;
    (void)offset;
    (void)flip;
#endif
}

void engine_profile_memory_alloc(const void* memory, size_t size, const char* name, int callstack_depth)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (memory == NULL || size == 0U || TracyCIsConnected == 0)
    {
        return;
    }

    if (!engine_profile_memory_tracker_add(memory))
    {
        return;
    }

    if (name != NULL)
    {
        if (callstack_depth > 0)
        {
            TracyCAllocNS(memory, size, callstack_depth, name);
        }
        else
        {
            TracyCAllocN(memory, size, name);
        }
    }
    else if (callstack_depth > 0)
    {
        TracyCAllocS(memory, size, callstack_depth);
    }
    else
    {
        TracyCAlloc(memory, size);
    }
#else
    (void)memory;
    (void)size;
    (void)name;
    (void)callstack_depth;
#endif
}

void engine_profile_memory_free(const void* memory, const char* name, int callstack_depth)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (memory == NULL || !engine_profile_memory_tracker_remove(memory))
    {
        return;
    }

    if (name != NULL)
    {
        if (callstack_depth > 0)
        {
            TracyCFreeNS(memory, callstack_depth, name);
        }
        else
        {
            TracyCFreeN(memory, name);
        }
    }
    else if (callstack_depth > 0)
    {
        TracyCFreeS(memory, callstack_depth);
    }
    else
    {
        TracyCFree(memory);
    }
#else
    (void)memory;
    (void)name;
    (void)callstack_depth;
#endif
}

EngineProfileLockCtx engine_profile_lock_create(const char* name)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    TracyCLockCtx lock;

    TracyCLockAnnounce(lock);
    if (name != NULL && name[0] != '\0')
    {
        TracyCLockCustomName(lock, name, strlen(name));
    }
    return lock;
#else
    (void)name;
    return NULL;
#endif
}

void engine_profile_lock_destroy(EngineProfileLockCtx lock)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (lock != NULL)
    {
        TracyCLockTerminate(lock);
    }
#else
    (void)lock;
#endif
}

void engine_profile_lock_before_lock(EngineProfileLockCtx lock)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (lock != NULL)
    {
        TracyCLockBeforeLock(lock);
    }
#else
    (void)lock;
#endif
}

void engine_profile_lock_after_lock(EngineProfileLockCtx lock)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (lock != NULL)
    {
        TracyCLockAfterLock(lock);
    }
#else
    (void)lock;
#endif
}

void engine_profile_lock_after_unlock(EngineProfileLockCtx lock)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (lock != NULL)
    {
        TracyCLockAfterUnlock(lock);
    }
#else
    (void)lock;
#endif
}

void engine_profile_lock_after_try_lock(EngineProfileLockCtx lock, bool acquired)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (lock != NULL)
    {
        TracyCLockAfterTryLock(lock, acquired ? 1 : 0);
    }
#else
    (void)lock;
    (void)acquired;
#endif
}

void engine_profile_lock_set_name(EngineProfileLockCtx lock, const char* name)
{
#if defined(JIMOKOMI_ENABLE_TRACY_PROFILE)
    if (lock != NULL && name != NULL && name[0] != '\0')
    {
        TracyCLockCustomName(lock, name, strlen(name));
    }
#else
    (void)lock;
    (void)name;
#endif
}
