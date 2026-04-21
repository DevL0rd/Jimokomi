#ifndef JIMOKOMI_ENGINE_RUNTIME_APP_RENDER_LOOP_H
#define JIMOKOMI_ENGINE_RUNTIME_APP_RENDER_LOOP_H

typedef struct EngineAppContext EngineAppContext;
typedef struct EngineSettings EngineSettings;
typedef struct InputPacketStream InputPacketStream;
typedef struct RenderSnapshotExchange RenderSnapshotExchange;

void engine_app_run_render_loop(
    EngineAppContext* app,
    const EngineSettings* settings,
    RenderSnapshotExchange* render_snapshot_exchange,
    InputPacketStream* input_stream
);

#endif
