#ifndef JIMOKOMI_ENGINE_RUNTIMECONFIG_H
#define JIMOKOMI_ENGINE_RUNTIMECONFIG_H

#include "Core/EngineConfig.h"
#include "Rendering/RendererConfig.h"
#include "Rendering/ResourceTypes.h"

#include <stdbool.h>

typedef struct EngineSettings EngineSettings;

typedef struct RuntimeConfig {
    int window_width;
    int window_height;
    const char* window_title;
    EngineConfig engine;
    RendererConfig renderer;
    BakePolicy default_procedural_bake_policy;
    bool default_prebake_required;
    bool default_bake_instance_invariant;
} RuntimeConfig;

void runtime_config_from_engine_settings(RuntimeConfig* config, const EngineSettings* settings);

#endif
