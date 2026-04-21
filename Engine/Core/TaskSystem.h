#ifndef JIMOKOMI_ENGINE_CORE_TASKSYSTEM_H
#define JIMOKOMI_ENGINE_CORE_TASKSYSTEM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct TaskSystem TaskSystem;
typedef void TaskSystemRangeCallback(int start_index, int end_index, uint32_t worker_index, void* user_context);

typedef struct TaskSystemConfig {
    int requested_worker_count;
} TaskSystemConfig;

typedef struct TaskSystemStatsSnapshot {
    int online_core_count;
    int worker_count;
    int background_thread_count;
    uint64_t enqueued_task_count;
    uint64_t inline_task_count;
    uint64_t main_chunk_count;
    uint64_t worker_chunk_count;
} TaskSystemStatsSnapshot;

bool task_system_init(TaskSystem* system, const TaskSystemConfig* config);
TaskSystem* task_system_create(const TaskSystemConfig* config);
void task_system_dispose(TaskSystem* system);
void task_system_destroy(TaskSystem* system);
int task_system_detect_online_core_count(void);
int task_system_get_online_core_count(const TaskSystem* system);
int task_system_get_worker_count(const TaskSystem* system);
void task_system_get_stats_snapshot(const TaskSystem* system, TaskSystemStatsSnapshot* out_snapshot);
bool task_system_parallel_for(
    const TaskSystem* system,
    int item_count,
    int min_range,
    TaskSystemRangeCallback* callback,
    void* user_context
);

#endif
