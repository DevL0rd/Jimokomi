#ifndef JIMOKOMI_ENGINE_CORE_ENGINECONFIG_H
#define JIMOKOMI_ENGINE_CORE_ENGINECONFIG_H

#include "Logger.h"
#include "Profiler.h"

#include <stdbool.h>

typedef struct EngineConfig {
    bool debug_stats;
    EngineLoggerConfig logger;
    EngineProfilerConfig profiler;
} EngineConfig;

#endif
