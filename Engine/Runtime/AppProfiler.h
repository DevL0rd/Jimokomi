#ifndef JIMOKOMI_ENGINE_APP_PROFILER_H
#define JIMOKOMI_ENGINE_APP_PROFILER_H

#include "../AppInternal.h"
#include "../Rendering/RenderTypes.h"

#include <stddef.h>

struct RenderSnapshotBuffer;

void engine_app_emit_profiler_metadata(
    EngineAppContext* app,
    const struct RenderSnapshotBuffer* render_snapshot,
    const RendererFrame* frame,
    size_t drained_resource_commands
);

#endif
