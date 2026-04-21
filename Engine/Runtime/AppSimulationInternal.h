#ifndef JIMOKOMI_ENGINE_RUNTIME_APP_SIMULATION_INTERNAL_H
#define JIMOKOMI_ENGINE_RUNTIME_APP_SIMULATION_INTERNAL_H

#include "AppSimulation.h"

#include "../AppInternal.h"
#include "../Core/InputPacketStream.h"
#include "../Rendering/RenderSnapshotExchange.h"
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
    uint64_t last_sleep_visibility_update_ms;
    struct Entity** sleep_visible_entities;
    size_t sleep_visible_entity_capacity;
    uint32_t max_substeps;
    bool cached_debug_overlay_enabled;
    bool cached_draw_debug_world;
} EngineAppSimThreadContext;

#endif
