#ifndef JIMOKOMI_ENGINE_CORE_TASKSYSTEM_H
#define JIMOKOMI_ENGINE_CORE_TASKSYSTEM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct TaskSystem TaskSystem;
typedef void TaskSystemRangeCallback(int start_index, int end_index, uint32_t worker_index, void* user_context);

typedef struct TaskSystemConfig {
    int requested_worker_count;
} TaskSystemConfig;

bool task_system_init(TaskSystem* system, const TaskSystemConfig* config);
void task_system_dispose(TaskSystem* system);
int task_system_detect_online_core_count(void);
int task_system_get_online_core_count(const TaskSystem* system);
int task_system_get_worker_count(const TaskSystem* system);
bool task_system_parallel_for(
    const TaskSystem* system,
    int item_count,
    int min_range,
    TaskSystemRangeCallback* callback,
    void* user_context
);

#endif
