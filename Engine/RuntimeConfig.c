#include "Engine/RuntimeConfig.h"
#include "Engine/Settings.h"

#include <string.h>

void runtime_config_init_defaults(RuntimeConfig* config) {
    const EngineSettings* settings = EngineSettings_GetDefaults();

    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->window_width = settings->window_width;
    config->window_height = settings->window_height;
    config->window_title = settings->window_title;

    config->engine.logger.path = settings->logger_path;
    config->engine.logger.max_lines = settings->logger_max_lines;
    config->engine.logger.flush_every = settings->logger_flush_every;
    config->engine.logger.echo_to_console = settings->logger_echo_to_console;
    config->engine.logger.minimum_level = ENGINE_LOG_LEVEL_TRACE;
    config->engine.profiler.enabled = settings->profiler_enabled;
    config->engine.profiler.path = settings->profiler_path;
    config->engine.profiler.text_path = settings->profiler_text_path;
    config->engine.profiler.max_frames = settings->profiler_max_frames;
    config->engine.profiler.flush_every = settings->profiler_flush_every;

    config->renderer.view_width = settings->renderer_view_width;
    config->renderer.view_height = settings->renderer_view_height;
    config->renderer.prebake_budget_per_frame = settings->renderer_prebake_budget_per_frame;
    config->renderer.prebake_admission_total_hits = settings->renderer_prebake_admission_total_hits;
    config->renderer.prebake_admission_frame_hits = settings->renderer_prebake_admission_frame_hits;

    config->default_procedural_bake_policy = BAKE_POLICY_SHARED_FRAME;
    config->default_prebake_required = true;
    config->default_bake_instance_invariant = true;
}
