#include "Engine/RuntimeConfig.h"
#include "Engine/Settings.h"

#include <string.h>

void runtime_config_from_engine_settings(RuntimeConfig* config, const EngineSettings* settings) {
    if (config == NULL) {
        return;
    }
    if (settings == NULL) {
        settings = EngineSettings_GetDefaults();
    }

    memset(config, 0, sizeof(*config));
    config->window_width = settings->window_width;
    config->window_height = settings->window_height;
    config->window_title = settings->window_title;

    config->renderer.view_width = settings->renderer_view_width;
    config->renderer.view_height = settings->renderer_view_height;
    config->renderer.prebake_target_fps = settings->renderer_prebake_target_fps;
    config->renderer.prebake_admission_total_hits = settings->renderer_prebake_admission_total_hits;
    config->renderer.prebake_admission_frame_hits = settings->renderer_prebake_admission_frame_hits;

    config->default_procedural_bake_policy = BAKE_POLICY_SHARED_FRAME;
    config->default_prebake_required = true;
    config->default_bake_instance_invariant = true;
}
