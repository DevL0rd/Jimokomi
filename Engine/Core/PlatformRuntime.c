#if !defined(_WIN32) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "PlatformRuntimeInternal.h"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

typedef struct EnginePlatformThreadStart
{
    EnginePlatformThreadMain* main;
    void* user_data;
} EnginePlatformThreadStart;

static DWORD WINAPI engine_platform_thread_entry(void* user_data)
{
    EnginePlatformThreadStart* start = (EnginePlatformThreadStart*)user_data;

    if (start != NULL)
    {
        EnginePlatformThreadMain* main = start->main;
        void* thread_user_data = start->user_data;
        free(start);
        if (main != NULL)
        {
            main(thread_user_data);
        }
    }

    return 0U;
}

bool engine_platform_thread_start(EnginePlatformThread* thread, EnginePlatformThreadMain* main, void* user_data)
{
    EnginePlatformThreadStart* start;

    if (thread == NULL || main == NULL)
    {
        return false;
    }

    start = (EnginePlatformThreadStart*)malloc(sizeof(*start));
    if (start == NULL)
    {
        return false;
    }

    start->main = main;
    start->user_data = user_data;
    thread->handle = CreateThread(NULL, 0, engine_platform_thread_entry, start, 0, NULL);
    if (thread->handle == NULL)
    {
        free(start);
        return false;
    }

    return true;
}

void engine_platform_thread_join(EnginePlatformThread* thread)
{
    if (thread == NULL || thread->handle == NULL)
    {
        return;
    }

    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    thread->handle = NULL;
}

void engine_platform_set_current_thread_name(const char* name)
{
    HRESULT(WINAPI * set_thread_description)(HANDLE, PCWSTR) = NULL;
    HMODULE kernel_module = NULL;
    int wide_length = 0;
    wchar_t* wide_name = NULL;

    if (name == NULL || name[0] == '\0')
    {
        return;
    }

    kernel_module = GetModuleHandleW(L"Kernel32.dll");
    if (kernel_module == NULL)
    {
        return;
    }

    set_thread_description = (HRESULT(WINAPI*)(HANDLE, PCWSTR))GetProcAddress(kernel_module, "SetThreadDescription");
    if (set_thread_description == NULL)
    {
        return;
    }

    wide_length = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (wide_length <= 0)
    {
        return;
    }

    wide_name = (wchar_t*)malloc((size_t)wide_length * sizeof(*wide_name));
    if (wide_name == NULL)
    {
        return;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, name, -1, wide_name, wide_length) > 0)
    {
        set_thread_description(GetCurrentThread(), wide_name);
    }

    free(wide_name);
}

bool engine_platform_mutex_init(EnginePlatformMutex* mutex)
{
    return engine_platform_mutex_init_named(mutex, NULL);
}

bool engine_platform_mutex_init_named(EnginePlatformMutex* mutex, const char* profile_name)
{
    if (mutex == NULL)
    {
        return false;
    }

    mutex->profile_lock = engine_profile_lock_create(profile_name != NULL ? profile_name : "Engine Mutex");
    if (InitializeCriticalSectionAndSpinCount(&mutex->handle, 4000U) == 0)
    {
        engine_profile_lock_destroy(mutex->profile_lock);
        mutex->profile_lock = NULL;
        return false;
    }

    return true;
}

void engine_platform_mutex_dispose(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        engine_profile_lock_destroy(mutex->profile_lock);
        mutex->profile_lock = NULL;
        DeleteCriticalSection(&mutex->handle);
    }
}

void engine_platform_mutex_lock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        engine_profile_lock_before_lock(mutex->profile_lock);
        EnterCriticalSection(&mutex->handle);
        engine_profile_lock_after_lock(mutex->profile_lock);
    }
}

void engine_platform_mutex_unlock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        engine_profile_lock_after_unlock(mutex->profile_lock);
        LeaveCriticalSection(&mutex->handle);
    }
}

bool engine_platform_condition_variable_init(EnginePlatformConditionVariable* condition_variable)
{
    if (condition_variable == NULL)
    {
        return false;
    }

    InitializeConditionVariable(&condition_variable->handle);
    return true;
}

void engine_platform_condition_variable_dispose(EnginePlatformConditionVariable* condition_variable)
{
    (void)condition_variable;
}

void engine_platform_condition_variable_wait(
    EnginePlatformConditionVariable* condition_variable,
    EnginePlatformMutex* mutex
)
{
    if (condition_variable != NULL && mutex != NULL)
    {
        SleepConditionVariableCS(&condition_variable->handle, &mutex->handle, INFINITE);
    }
}

void engine_platform_condition_variable_broadcast(EnginePlatformConditionVariable* condition_variable)
{
    if (condition_variable != NULL)
    {
        WakeAllConditionVariable(&condition_variable->handle);
    }
}

int engine_platform_get_online_core_count(void)
{
    SYSTEM_INFO system_info;
    DWORD cores;

    GetSystemInfo(&system_info);
    cores = system_info.dwNumberOfProcessors;
    if (cores < 1U)
    {
        return 1;
    }
    if (cores > 1024U)
    {
        return 1024;
    }

    return (int)cores;
}

double engine_platform_now_ms(void)
{
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;

    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
    {
        return (double)GetTickCount64();
    }

    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
}

void engine_platform_sleep_ms(uint32_t milliseconds)
{
    Sleep((DWORD)milliseconds);
}

#else

#include <errno.h>
#include <time.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <pthread.h>
#else
#include <pthread.h>
#endif

bool engine_platform_thread_start(EnginePlatformThread* thread, EnginePlatformThreadMain* main, void* user_data)
{
    if (thread == NULL || main == NULL)
    {
        return false;
    }

    return pthread_create(&thread->handle, NULL, main, user_data) == 0;
}

void engine_platform_thread_join(EnginePlatformThread* thread)
{
    if (thread != NULL)
    {
        pthread_join(thread->handle, NULL);
    }
}

void engine_platform_set_current_thread_name(const char* name)
{
    if (name == NULL || name[0] == '\0')
    {
        return;
    }

#if defined(__APPLE__)
    pthread_setname_np(name);
#else
    char truncated_name[16];

    memset(truncated_name, 0, sizeof(truncated_name));
    strncpy(truncated_name, name, sizeof(truncated_name) - 1U);
    pthread_setname_np(pthread_self(), truncated_name);
#endif
}

bool engine_platform_mutex_init(EnginePlatformMutex* mutex)
{
    return engine_platform_mutex_init_named(mutex, NULL);
}

bool engine_platform_mutex_init_named(EnginePlatformMutex* mutex, const char* profile_name)
{
    if (mutex == NULL)
    {
        return false;
    }

    mutex->profile_lock = engine_profile_lock_create(profile_name != NULL ? profile_name : "Engine Mutex");
    if (pthread_mutex_init(&mutex->handle, NULL) != 0)
    {
        engine_profile_lock_destroy(mutex->profile_lock);
        mutex->profile_lock = NULL;
        return false;
    }

    return true;
}

void engine_platform_mutex_dispose(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        engine_profile_lock_destroy(mutex->profile_lock);
        mutex->profile_lock = NULL;
        pthread_mutex_destroy(&mutex->handle);
    }
}

void engine_platform_mutex_lock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        engine_profile_lock_before_lock(mutex->profile_lock);
        pthread_mutex_lock(&mutex->handle);
        engine_profile_lock_after_lock(mutex->profile_lock);
    }
}

void engine_platform_mutex_unlock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        engine_profile_lock_after_unlock(mutex->profile_lock);
        pthread_mutex_unlock(&mutex->handle);
    }
}

bool engine_platform_condition_variable_init(EnginePlatformConditionVariable* condition_variable)
{
    return condition_variable != NULL && pthread_cond_init(&condition_variable->handle, NULL) == 0;
}

void engine_platform_condition_variable_dispose(EnginePlatformConditionVariable* condition_variable)
{
    if (condition_variable != NULL)
    {
        pthread_cond_destroy(&condition_variable->handle);
    }
}

void engine_platform_condition_variable_wait(
    EnginePlatformConditionVariable* condition_variable,
    EnginePlatformMutex* mutex
)
{
    if (condition_variable != NULL && mutex != NULL)
    {
        pthread_cond_wait(&condition_variable->handle, &mutex->handle);
    }
}

void engine_platform_condition_variable_broadcast(EnginePlatformConditionVariable* condition_variable)
{
    if (condition_variable != NULL)
    {
        pthread_cond_broadcast(&condition_variable->handle);
    }
}

int engine_platform_get_online_core_count(void)
{
    long cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (cores < 1)
    {
        return 1;
    }
    if (cores > 1024)
    {
        return 1024;
    }

    return (int)cores;
}

double engine_platform_now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

void engine_platform_sleep_ms(uint32_t milliseconds)
{
    struct timespec request;

    request.tv_sec = (time_t)(milliseconds / 1000U);
    request.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    while (nanosleep(&request, &request) != 0 && errno == EINTR)
    {
    }
}

#endif
