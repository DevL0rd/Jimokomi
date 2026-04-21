#ifndef JIMOKOMI_ENGINE_CORE_TASKSYSTEM_INTERNAL_H
#define JIMOKOMI_ENGINE_CORE_TASKSYSTEM_INTERNAL_H

#include "TaskSystem.h"

struct TaskSystem {
    int online_core_count;
    int worker_count;
    void* implementation;
};

#endif
