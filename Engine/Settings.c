#include "Settings.h"

const EngineSettings* EngineSettings_GetDefaults(void)
{
    static const EngineSettings settings = {
        .window_width = 960,
        .window_height = 540,
        .window_title = "Jimokomi",
        .vsync_enabled = true,

        .app_snapshot_preload_screens = 1.0f,
        .app_snapshot_publish_interval_ms = 16ULL,
        .app_idle_sleep_ms = 1U,
        .app_clear_color = { 0x0b1020U },

        .scene_spatial_grid_cell_size = 64.0f,

        .physics_onscreen_sleep_threshold = 0.05f,
        .physics_offscreen_sleep_threshold = 1.0f,
        .physics_adaptive_hz_enabled = false,
        .physics_target_hz = 60.0f,
        .physics_min_hz = 60.0f,
        .physics_max_hz = 300.0f,
        .physics_frame_budget_hz = 30.0f,
        .physics_max_substeps = 10U,
        .physics_step_substep_count = 4U,

        .camera_pan_key_speed = 640.0f,
        .camera_zoom_step = 0.12f,
        .camera_zoom_key_speed = 5.0f,
        .camera_pan_click_threshold = 4.0f,
        .camera_min_view_width = 160.0f,
        .camera_min_view_height = 90.0f,

        .debug_overlay_title_bar_height = 22.0f,
        .debug_overlay_inspector_collapsed_width = 26.0f,
        .debug_overlay_history_push_interval_ms = 100ULL,
        .debug_overlay_dashboard_redraw_interval_ms = 100ULL,
        .debug_overlay_inspector_redraw_interval_ms = 100ULL
    };

    return &settings;
}
