#ifndef JIMOKOMI_ENGINE_ENGINE_H
#define JIMOKOMI_ENGINE_ENGINE_H

#include <stdbool.h>

#include "Core/Input.h"
#include "Core/Logger.h"
#include "Core/Profiler.h"
#include "Core/Stats.h"

typedef struct EngineConfig {
    bool debug_stats;
    EngineLoggerConfig logger;
    EngineProfilerConfig profiler;
} EngineConfig;

typedef struct Engine {
    EngineConfig config;
    EngineInput input;
    EngineLogger logger;
    EngineProfiler profiler;
    EngineStats stats;
    bool initialized;
} Engine;

bool Engine_init(Engine* engine, const EngineConfig* config);
void Engine_update(Engine* engine, double dt, const EngineInputSnapshot* input_snapshot);
void Engine_draw_begin(Engine* engine);
void Engine_draw_end(Engine* engine);
EngineStatsSnapshot Engine_get_stats_snapshot(const Engine* engine);
void Engine_dispose(Engine* engine);

#endif
