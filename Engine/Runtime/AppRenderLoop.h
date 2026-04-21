#ifndef JIMOKOMI_ENGINE_RUNTIME_APP_RENDER_LOOP_H
#define JIMOKOMI_ENGINE_RUNTIME_APP_RENDER_LOOP_H

#include "../AppInternal.h"
#include "../Core/InputPacketStream.h"
#include "../Rendering/RenderSnapshot.h"
#include "../Settings.h"

void engine_app_run_render_loop(
    EngineAppContext* app,
    const EngineSettings* settings,
    RenderSnapshotExchange* render_snapshot_exchange,
    InputPacketStream* input_stream
);

#endif
