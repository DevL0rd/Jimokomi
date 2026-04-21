#ifndef JIMOKOMI_ENGINE_RUNTIMECONFIG_H
#define JIMOKOMI_ENGINE_RUNTIMECONFIG_H

#include "Core/EngineConfig.h"

#include <stdbool.h>

typedef struct EngineSettings EngineSettings;

typedef struct RuntimeConfig {
    int window_width;
    int window_height;
    const char* window_title;
    bool vsync_enabled;
    EngineConfig engine;
} RuntimeConfig;

void runtime_config_from_engine_settings(RuntimeConfig* config, const EngineSettings* settings);

#endif
