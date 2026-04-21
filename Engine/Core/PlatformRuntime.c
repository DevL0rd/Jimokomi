#include "PlatformRuntimeInternal.h"

#include <stdlib.h>

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

bool engine_platform_mutex_init(EnginePlatformMutex* mutex)
{
    if (mutex == NULL)
    {
        return false;
    }

    return InitializeCriticalSectionAndSpinCount(&mutex->handle, 4000U) != 0;
}

void engine_platform_mutex_dispose(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        DeleteCriticalSection(&mutex->handle);
    }
}

void engine_platform_mutex_lock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        EnterCriticalSection(&mutex->handle);
    }
}

void engine_platform_mutex_unlock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
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

bool engine_platform_mutex_init(EnginePlatformMutex* mutex)
{
    return mutex != NULL && pthread_mutex_init(&mutex->handle, NULL) == 0;
}

void engine_platform_mutex_dispose(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        pthread_mutex_destroy(&mutex->handle);
    }
}

void engine_platform_mutex_lock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
        pthread_mutex_lock(&mutex->handle);
    }
}

void engine_platform_mutex_unlock(EnginePlatformMutex* mutex)
{
    if (mutex != NULL)
    {
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
