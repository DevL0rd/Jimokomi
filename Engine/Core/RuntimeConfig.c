#include "RuntimeConfig.h"

#include <string.h>

void runtime_config_init_defaults(RuntimeConfig* config) {
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->window_width = 960;
    config->window_height = 540;
    config->window_title = "Jimokomi";

    config->engine.logger.path = "logs/runtime.log";
    config->engine.logger.max_lines = 800;
    config->engine.logger.flush_every = 1;
    config->engine.logger.echo_to_console = true;
    config->engine.logger.minimum_level = ENGINE_LOG_LEVEL_TRACE;
    config->engine.profiler.path = "logs/performance_profile.bin";
    config->engine.profiler.text_path = "logs/performance_profile.txt";
    config->engine.profiler.max_frames = 180;
    config->engine.profiler.flush_every = 15;

    config->renderer.view_width = 960;
    config->renderer.view_height = 540;
    config->renderer.prebake_budget_per_frame = 20U;
    config->renderer.prebake_admission_total_hits = 1U;
    config->renderer.prebake_admission_frame_hits = 1U;

    config->default_procedural_bake_policy = BAKE_POLICY_SHARED_FRAME;
    config->default_bake_instance_invariant = true;
}
