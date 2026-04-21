#include "Engine.h"

#include <string.h>

static EngineConfig Engine_default_config(void) {
    EngineConfig config;

    memset(&config, 0, sizeof(config));
    config.debug_stats = false;
    config.logger.path = "logs/runtime.log";
    config.logger.max_lines = 800;
    config.logger.flush_every = 1;
    config.logger.echo_to_console = true;
    config.logger.minimum_level = ENGINE_LOG_LEVEL_TRACE;
    config.profiler.enabled = true;
    config.profiler.path = "logs/performance_profile.bin";
    config.profiler.text_path = "logs/performance_profile.txt";
    config.profiler.max_frames = 180;
    config.profiler.flush_every = 120;
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
        if (config->logger.path != 0) resolved.logger.path = config->logger.path;
        if (config->logger.max_lines > 0) resolved.logger.max_lines = config->logger.max_lines;
        if (config->logger.flush_every > 0) resolved.logger.flush_every = config->logger.flush_every;
        resolved.logger.echo_to_console = config->logger.echo_to_console;
        if (config->logger.minimum_level > 0) resolved.logger.minimum_level = config->logger.minimum_level;
        if (config->profiler.path != 0) resolved.profiler.path = config->profiler.path;
        if (config->profiler.text_path != 0) resolved.profiler.text_path = config->profiler.text_path;
        if (config->profiler.max_frames > 0) resolved.profiler.max_frames = config->profiler.max_frames;
        if (config->profiler.flush_every > 0) resolved.profiler.flush_every = config->profiler.flush_every;
    }

    memset(engine, 0, sizeof(*engine));
    engine->config = resolved;

    if (!EngineLogger_init(&engine->logger, &resolved.logger)) {
        Engine_dispose(engine);
        return false;
    }

    if (!EngineProfiler_init(&engine->profiler, &resolved.profiler)) {
        Engine_dispose(engine);
        return false;
    }

    EngineInputBindings_set_defaults(&bindings);
    EngineInput_init(&engine->input, &bindings);
    EngineStats_init(&engine->stats);

    engine->initialized = true;
    EngineLogger_info(&engine->logger, "engine initialized", 0);
    return true;
}

void Engine_update(Engine* engine, double dt, const EngineInputSnapshot* input_snapshot) {
    (void)dt;

    if (engine == 0 || !engine->initialized) {
        return;
    }

    EngineProfiler_begin_frame(&engine->profiler, "update_callback");

    EngineProfiler_begin_scope(&engine->profiler, "engine.input");
    if (input_snapshot != 0) {
        EngineInput_capture(&engine->input, input_snapshot);
    }
    EngineProfiler_end_scope(&engine->profiler, "engine.input");

    EngineStats_begin_update(&engine->stats);
    EngineProfiler_begin_scope(&engine->profiler, "engine.frame");
    EngineStats_end_update(&engine->stats);
    EngineProfiler_end_scope(&engine->profiler, "engine.frame");

    EngineProfiler_set_metadata_number(&engine->profiler, "stats_update_ms", engine->stats.last_update_ms);
    EngineProfiler_end_frame(&engine->profiler);
}

void Engine_draw_begin(Engine* engine) {
    if (engine == 0 || !engine->initialized) {
        return;
    }

    EngineProfiler_begin_frame(&engine->profiler, "draw_callback");
    EngineStats_begin_draw(&engine->stats);
}

void Engine_draw_end(Engine* engine) {
    if (engine == 0 || !engine->initialized) {
        return;
    }

    EngineStats_end_draw(&engine->stats);
    EngineProfiler_set_metadata_number(&engine->profiler, "stats_draw_ms", engine->stats.last_draw_ms);
    EngineProfiler_set_metadata_number(&engine->profiler, "stats_frame_ms", engine->stats.last_frame_ms);
    EngineProfiler_end_frame(&engine->profiler);
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

    EngineProfiler_dispose(&engine->profiler);
    EngineLogger_dispose(&engine->logger);
    memset(engine, 0, sizeof(*engine));
}
