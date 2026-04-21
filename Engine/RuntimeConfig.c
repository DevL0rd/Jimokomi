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
}
