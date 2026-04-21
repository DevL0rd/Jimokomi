#include "TaskSystemInternal.h"

#include <box2d/box2d.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct TaskSystemTask {
    b2TaskCallback* callback;
    void* task_context;
    int item_count;
    int chunk_size;
    atomic_int next_index;
    atomic_int remaining_chunks;
    bool completed;
    pthread_mutex_t mutex;
    pthread_cond_t completed_cond;
    struct TaskSystemTask* next;
} TaskSystemTask;

typedef struct TaskSystemImpl TaskSystemImpl;

typedef struct TaskSystemWorkerContext {
    TaskSystemImpl* system;
    uint32_t worker_index;
} TaskSystemWorkerContext;

struct TaskSystemImpl {
    int worker_count;
    int background_thread_count;
    bool shutting_down;
    pthread_t* threads;
    TaskSystemWorkerContext* worker_contexts;
    pthread_mutex_t mutex;
    pthread_cond_t work_available_cond;
    TaskSystemTask* tasks_head;
};

static int task_system_detect_online_cores(void) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores < 1) {
        return 1;
    }
    if (cores > 1024) {
        return 1024;
    }
    return (int)cores;
}

static int task_system_default_worker_count(int online_cores) {
    int workers;

    if (online_cores < 1) {
        online_cores = 1;
    }

    workers = online_cores - 1;
    if (workers < 1) {
        workers = 1;
    }

    return workers;
}

static int task_system_compute_chunk_size(int item_count, int min_range, int worker_count) {
    int range = min_range > 0 ? min_range : 1;
    int chunk_size;

    if (item_count <= range) {
        return item_count;
    }

    chunk_size = (item_count + worker_count - 1) / worker_count;
    if (chunk_size < range) {
        chunk_size = range;
    }

    return chunk_size;
}

static bool task_system_has_claimable_work_locked(const TaskSystemImpl* system) {
    const TaskSystemTask* task = system != NULL ? system->tasks_head : NULL;

    while (task != NULL) {
        if (!task->completed && atomic_load(&task->next_index) < task->item_count) {
            return true;
        }
        task = task->next;
    }

    return false;
}

static TaskSystemTask* task_system_pick_task_locked(TaskSystemImpl* system) {
    TaskSystemTask* task = system != NULL ? system->tasks_head : NULL;

    while (task != NULL) {
        if (!task->completed && atomic_load(&task->next_index) < task->item_count) {
            return task;
        }
        task = task->next;
    }

    return NULL;
}

static void* task_system_worker_main(void* user_data) {
    TaskSystemWorkerContext* worker = (TaskSystemWorkerContext*)user_data;
    TaskSystemImpl* system = worker != NULL ? worker->system : NULL;

    if (worker == NULL || system == NULL) {
        return NULL;
    }

    for (;;) {
        TaskSystemTask* task;
        int start_index;
        int end_index;

        pthread_mutex_lock(&system->mutex);
        while (!system->shutting_down && !task_system_has_claimable_work_locked(system)) {
            pthread_cond_wait(&system->work_available_cond, &system->mutex);
        }

        if (system->shutting_down) {
            pthread_mutex_unlock(&system->mutex);
            break;
        }

        task = task_system_pick_task_locked(system);
        pthread_mutex_unlock(&system->mutex);

        if (task == NULL) {
            continue;
        }

        start_index = atomic_fetch_add(&task->next_index, task->chunk_size);
        if (start_index >= task->item_count) {
            continue;
        }

        end_index = start_index + task->chunk_size;
        if (end_index > task->item_count) {
            end_index = task->item_count;
        }

        task->callback(start_index, end_index, worker->worker_index, task->task_context);

        if (atomic_fetch_sub(&task->remaining_chunks, 1) == 1) {
            pthread_mutex_lock(&task->mutex);
            task->completed = true;
            pthread_cond_broadcast(&task->completed_cond);
            pthread_mutex_unlock(&task->mutex);
        }
    }

    return NULL;
}

static void* task_system_enqueue_task(
    b2TaskCallback* task,
    int item_count,
    int min_range,
    void* task_context,
    void* user_context
) {
    TaskSystemImpl* system = (TaskSystemImpl*)user_context;
    TaskSystemTask* user_task;
    int chunk_size;
    int chunk_count;

    if (system == NULL || task == NULL || item_count <= 0) {
        return NULL;
    }

    chunk_size = task_system_compute_chunk_size(item_count, min_range, system->worker_count);
    chunk_count = (item_count + chunk_size - 1) / chunk_size;

    if (chunk_count <= 1 || system->worker_count <= 1) {
        task(0, item_count, 0U, task_context);
        return NULL;
    }

    user_task = (TaskSystemTask*)calloc(1, sizeof(*user_task));
    if (user_task == NULL) {
        task(0, item_count, 0U, task_context);
        return NULL;
    }

    user_task->callback = task;
    user_task->task_context = task_context;
    user_task->item_count = item_count;
    user_task->chunk_size = chunk_size;
    atomic_init(&user_task->next_index, 0);
    atomic_init(&user_task->remaining_chunks, chunk_count);
    pthread_mutex_init(&user_task->mutex, NULL);
    pthread_cond_init(&user_task->completed_cond, NULL);

    pthread_mutex_lock(&system->mutex);
    user_task->next = system->tasks_head;
    system->tasks_head = user_task;
    pthread_cond_broadcast(&system->work_available_cond);
    pthread_mutex_unlock(&system->mutex);

    return user_task;
}

static void task_system_finish_task(void* user_task, void* user_context) {
    TaskSystemTask* task = (TaskSystemTask*)user_task;
    TaskSystemImpl* system = (TaskSystemImpl*)user_context;
    TaskSystemTask** cursor;

    if (task == NULL || system == NULL) {
        return;
    }

    for (;;) {
        int start_index = atomic_fetch_add(&task->next_index, task->chunk_size);
        int end_index;

        if (start_index >= task->item_count) {
            break;
        }

        end_index = start_index + task->chunk_size;
        if (end_index > task->item_count) {
            end_index = task->item_count;
        }

        task->callback(start_index, end_index, 0U, task->task_context);

        if (atomic_fetch_sub(&task->remaining_chunks, 1) == 1) {
            pthread_mutex_lock(&task->mutex);
            task->completed = true;
            pthread_cond_broadcast(&task->completed_cond);
            pthread_mutex_unlock(&task->mutex);
            break;
        }
    }

    pthread_mutex_lock(&task->mutex);
    while (!task->completed) {
        pthread_cond_wait(&task->completed_cond, &task->mutex);
    }
    pthread_mutex_unlock(&task->mutex);

    pthread_mutex_lock(&system->mutex);
    cursor = &system->tasks_head;
    while (*cursor != NULL) {
        if (*cursor == task) {
            *cursor = task->next;
            break;
        }
        cursor = &(*cursor)->next;
    }
    pthread_mutex_unlock(&system->mutex);

    pthread_cond_destroy(&task->completed_cond);
    pthread_mutex_destroy(&task->mutex);
    free(task);
}

int task_system_detect_online_core_count(void) {
    return task_system_detect_online_cores();
}

bool task_system_init(TaskSystem* system, const TaskSystemConfig* config) {
    TaskSystemImpl* implementation;
    int online_cores;
    int requested_workers;
    int index;

    if (system == NULL) {
        return false;
    }

    memset(system, 0, sizeof(*system));
    online_cores = task_system_detect_online_cores();
    requested_workers = config != NULL ? config->requested_worker_count : 0;
    if (requested_workers < 1) {
        requested_workers = task_system_default_worker_count(online_cores);
    }
    if (requested_workers < 1) {
        requested_workers = 1;
    }

    implementation = (TaskSystemImpl*)calloc(1, sizeof(*implementation));
    if (implementation == NULL) {
        return false;
    }

    implementation->worker_count = requested_workers;
    implementation->background_thread_count = requested_workers > 1 ? requested_workers - 1 : 0;
    if (implementation->background_thread_count > 0) {
        implementation->threads = (pthread_t*)calloc((size_t)implementation->background_thread_count, sizeof(*implementation->threads));
        implementation->worker_contexts = (TaskSystemWorkerContext*)calloc((size_t)implementation->background_thread_count, sizeof(*implementation->worker_contexts));
    }
    if ((implementation->background_thread_count > 0) &&
        (implementation->threads == NULL || implementation->worker_contexts == NULL)) {
        free(implementation->worker_contexts);
        free(implementation->threads);
        free(implementation);
        return false;
    }

    pthread_mutex_init(&implementation->mutex, NULL);
    pthread_cond_init(&implementation->work_available_cond, NULL);

    for (index = 0; index < implementation->background_thread_count; ++index) {
        implementation->worker_contexts[index].system = implementation;
        implementation->worker_contexts[index].worker_index = (uint32_t)(index + 1);
        if (pthread_create(
            &implementation->threads[index],
            NULL,
            task_system_worker_main,
            &implementation->worker_contexts[index]
        ) != 0) {
            implementation->background_thread_count = index;
            implementation->shutting_down = true;
            pthread_cond_broadcast(&implementation->work_available_cond);
            while (--index >= 0) {
                pthread_join(implementation->threads[index], NULL);
            }
            pthread_cond_destroy(&implementation->work_available_cond);
            pthread_mutex_destroy(&implementation->mutex);
            free(implementation->worker_contexts);
            free(implementation->threads);
            free(implementation);
            return false;
        }
    }

    system->online_core_count = online_cores;
    system->worker_count = requested_workers;
    system->implementation = implementation;
    return true;
}

void task_system_dispose(TaskSystem* system) {
    TaskSystemImpl* implementation;
    TaskSystemTask* task;
    int index;

    if (system == NULL || system->implementation == NULL) {
        return;
    }

    implementation = (TaskSystemImpl*)system->implementation;

    pthread_mutex_lock(&implementation->mutex);
    implementation->shutting_down = true;
    pthread_cond_broadcast(&implementation->work_available_cond);
    pthread_mutex_unlock(&implementation->mutex);

    for (index = 0; index < implementation->background_thread_count; ++index) {
        pthread_join(implementation->threads[index], NULL);
    }

    task = implementation->tasks_head;
    while (task != NULL) {
        TaskSystemTask* next = task->next;
        pthread_cond_destroy(&task->completed_cond);
        pthread_mutex_destroy(&task->mutex);
        free(task);
        task = next;
    }

    pthread_cond_destroy(&implementation->work_available_cond);
    pthread_mutex_destroy(&implementation->mutex);
    free(implementation->worker_contexts);
    free(implementation->threads);
    free(implementation);

    memset(system, 0, sizeof(*system));
}

int task_system_get_online_core_count(const TaskSystem* system) {
    return system != NULL ? system->online_core_count : 0;
}

int task_system_get_worker_count(const TaskSystem* system) {
    return system != NULL ? system->worker_count : 0;
}

bool task_system_parallel_for(
    const TaskSystem* system,
    int item_count,
    int min_range,
    TaskSystemRangeCallback* callback,
    void* user_context
) {
    TaskSystemImpl* implementation;
    TaskSystemTask* task;

    if (system == NULL || system->implementation == NULL || callback == NULL || item_count <= 0) {
        return false;
    }

    implementation = (TaskSystemImpl*)system->implementation;
    task = (TaskSystemTask*)task_system_enqueue_task(
        (b2TaskCallback*)callback,
        item_count,
        min_range,
        user_context,
        implementation
    );
    if (task != NULL) {
        task_system_finish_task(task, implementation);
    }

    return true;
}

void task_system_configure_box2d_world_def(const TaskSystem* system, b2WorldDef* world_def) {
    if (system == NULL || world_def == NULL || system->implementation == NULL || system->worker_count < 1) {
        return;
    }

    world_def->workerCount = system->worker_count;
    world_def->enqueueTask = task_system_enqueue_task;
    world_def->finishTask = task_system_finish_task;
    world_def->userTaskContext = system->implementation;
}
