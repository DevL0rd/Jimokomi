#include "Settings.h"

const EngineSettings* EngineSettings_GetDefaults(void)
{
    static const EngineSettings settings = {
        .window_width = 960,
        .window_height = 540,
        .window_title = "Jimokomi",

        .logger_path = "logs/runtime.log",
        .logger_max_lines = 800U,
        .logger_flush_every = 1U,
        .logger_echo_to_console = true,

        .profiler_enabled = false,
        .profiler_path = "logs/performance_profile.bin",
        .profiler_text_path = "logs/performance_profile.txt",
        .profiler_max_frames = 180U,
        .profiler_flush_every = 120U,

        .renderer_view_width = 960,
        .renderer_view_height = 540,
        .renderer_prebake_target_fps = 60.0f,
        .renderer_prebake_admission_total_hits = 1U,
        .renderer_prebake_admission_frame_hits = 1U,

        .app_snapshot_preload_screens = 1.0f,
        .app_snapshot_publish_interval_ms = 16ULL,
        .app_idle_sleep_ms = 1U,
        .app_clear_color = { 0x0b1020U },

        .scene_spatial_grid_cell_size = 64.0f,

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
