#ifndef JIMOKOMI_ENGINE_RUNTIME_APP_SIMULATION_H
#define JIMOKOMI_ENGINE_RUNTIME_APP_SIMULATION_H

#include "../AppInternal.h"
#include "../Core/InputPacketStream.h"
#include "../Rendering/RenderSnapshot.h"
#include "../Settings.h"

#include <stdatomic.h>

typedef struct EngineAppSimThreadContext
{
    EngineAppContext* app;
    const EngineAppDesc* desc;
    const EngineSettings* settings;
    RenderSnapshotExchange* render_snapshot_exchange;
    InputPacketStream* input_stream;
    atomic_bool shutdown_requested;
    double accumulator_seconds;
    double fixed_dt_seconds;
    uint32_t max_substeps;
    bool cached_debug_overlay_enabled;
    bool cached_draw_debug_world;
} EngineAppSimThreadContext;

void* engine_app_simulation_thread_main(void* user_data);

#endif
