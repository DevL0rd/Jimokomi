#include "Engine.h"

#include <string.h>

static EngineConfig Engine_default_config(void) {
    EngineConfig config;

    memset(&config, 0, sizeof(config));
    config.debug_stats = false;
    return config;
}

bool Engine_init(Engine* engine, const EngineConfig* config) {
    EngineConfig resolved;
    EngineInputBindings bindings;

    if (engine == 0) {
        return false;
    }

    resolved = Engine_default_config();
    if (config != 0) {
        resolved.debug_stats = config->debug_stats;
    }

    memset(engine, 0, sizeof(*engine));
    engine->config = resolved;

    EngineInputBindings_set_defaults(&bindings);
    EngineInput_init(&engine->input, &bindings);
    EngineStats_init(&engine->stats);

    engine->initialized = true;
    return true;
}

void Engine_update(Engine* engine, double dt, const EngineInputSnapshot* input_snapshot) {
    (void)dt;

    if (engine == 0 || !engine->initialized) {
        return;
    }

    if (input_snapshot != 0) {
        EngineInput_capture(&engine->input, input_snapshot);
    }

    EngineStats_begin_update(&engine->stats);
    EngineStats_end_update(&engine->stats);
}

void Engine_draw_begin(Engine* engine) {
    if (engine == 0 || !engine->initialized) {
        return;
    }

    EngineStats_begin_draw(&engine->stats);
}

void Engine_draw_end(Engine* engine) {
    if (engine == 0 || !engine->initialized) {
        return;
    }

    EngineStats_end_draw(&engine->stats);
}

EngineStatsSnapshot Engine_get_stats_snapshot(const Engine* engine) {
    if (engine == 0) {
        EngineStatsSnapshot empty;
        memset(&empty, 0, sizeof(empty));
        return empty;
    }
    return EngineStats_get_snapshot(&engine->stats);
}

void Engine_dispose(Engine* engine) {
    if (engine == 0) {
        return;
    }

    memset(engine, 0, sizeof(*engine));
}
