#ifndef JIMOKOMI_ENGINE_CORE_RUNTIMECONFIG_H
#define JIMOKOMI_ENGINE_CORE_RUNTIMECONFIG_H

#include "../Rendering/ResourceManager.h"
#include "../Rendering/Renderer.h"
#include "../Engine.h"

typedef struct RuntimeConfig {
    int window_width;
    int window_height;
    const char* window_title;
    EngineConfig engine;
    RendererConfig renderer;
    BakePolicy default_procedural_bake_policy;
    bool default_bake_instance_invariant;
} RuntimeConfig;

void runtime_config_init_defaults(RuntimeConfig* config);

#endif
