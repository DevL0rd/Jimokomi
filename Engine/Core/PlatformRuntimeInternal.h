#ifndef JIMOKOMI_ENGINE_CORE_PLATFORM_RUNTIME_INTERNAL_H
#define JIMOKOMI_ENGINE_CORE_PLATFORM_RUNTIME_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "Profiling.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef void* EnginePlatformThreadMain(void* user_data);

typedef struct EnginePlatformThread
{
#if defined(_WIN32)
    HANDLE handle;
#else
    pthread_t handle;
#endif
} EnginePlatformThread;

typedef struct EnginePlatformMutex
{
#if defined(_WIN32)
    CRITICAL_SECTION handle;
#else
    pthread_mutex_t handle;
#endif
    EngineProfileLockCtx profile_lock;
} EnginePlatformMutex;

typedef struct EnginePlatformConditionVariable
{
#if defined(_WIN32)
    CONDITION_VARIABLE handle;
#else
    pthread_cond_t handle;
#endif
} EnginePlatformConditionVariable;

bool engine_platform_thread_start(EnginePlatformThread* thread, EnginePlatformThreadMain* main, void* user_data);
void engine_platform_thread_join(EnginePlatformThread* thread);
void engine_platform_set_current_thread_name(const char* name);

bool engine_platform_mutex_init(EnginePlatformMutex* mutex);
bool engine_platform_mutex_init_named(EnginePlatformMutex* mutex, const char* profile_name);
void engine_platform_mutex_dispose(EnginePlatformMutex* mutex);
void engine_platform_mutex_lock(EnginePlatformMutex* mutex);
void engine_platform_mutex_unlock(EnginePlatformMutex* mutex);

bool engine_platform_condition_variable_init(EnginePlatformConditionVariable* condition_variable);
void engine_platform_condition_variable_dispose(EnginePlatformConditionVariable* condition_variable);
void engine_platform_condition_variable_wait(
    EnginePlatformConditionVariable* condition_variable,
    EnginePlatformMutex* mutex
);
void engine_platform_condition_variable_broadcast(EnginePlatformConditionVariable* condition_variable);

int engine_platform_get_online_core_count(void);
double engine_platform_now_ms(void);
void engine_platform_sleep_ms(uint32_t milliseconds);

#endif
