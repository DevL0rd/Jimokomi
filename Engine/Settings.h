#ifndef JIMOKOMI_ENGINE_SETTINGS_H
#define JIMOKOMI_ENGINE_SETTINGS_H

#include "Rendering/RenderCommon.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct EngineSettings
{
    int window_width;
    int window_height;
    const char* window_title;
    bool vsync_enabled;

    float app_snapshot_preload_screens;
    uint64_t app_snapshot_publish_interval_ms;
    uint32_t app_idle_sleep_ms;
    Color32 app_clear_color;

    float scene_spatial_grid_cell_size;

    float camera_pan_key_speed;
    float camera_zoom_step;
    float camera_zoom_key_speed;
    float camera_pan_click_threshold;
    float camera_min_view_width;
    float camera_min_view_height;

    float debug_overlay_title_bar_height;
    float debug_overlay_inspector_collapsed_width;
    uint64_t debug_overlay_history_push_interval_ms;
    uint64_t debug_overlay_dashboard_redraw_interval_ms;
    uint64_t debug_overlay_inspector_redraw_interval_ms;
} EngineSettings;

const EngineSettings* EngineSettings_GetDefaults(void);

#endif
