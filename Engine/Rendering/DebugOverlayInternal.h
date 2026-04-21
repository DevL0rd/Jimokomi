#ifndef JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_DEBUGOVERLAY_INTERNAL_H

#include "DebugOverlay.h"

#define DEBUG_OVERLAY_HISTORY_CAPACITY 64

enum {
    DEBUG_UI_HOVER_NONE = 0,
    DEBUG_UI_HOVER_DASHBOARD = 1,
    DEBUG_UI_HOVER_INSPECTOR = 2
};

typedef struct DebugPanelState {
    float x;
    float y;
    float width;
    float height;
    bool dragging;
    float drag_offset_x;
    float drag_offset_y;
} DebugPanelState;

typedef struct DebugOverlayUiState DebugOverlayUiState;
typedef struct DebugOverlayDashboardState DebugOverlayDashboardState;
typedef struct DebugOverlayInspectorState DebugOverlayInspectorState;
typedef struct DebugOverlayWorldState DebugOverlayWorldState;
typedef struct DebugOverlayHistoryState DebugOverlayHistoryState;

typedef struct DebugOverlay {
    DebugOverlayUiState* ui;
    DebugOverlayDashboardState* dashboard;
    DebugOverlayInspectorState* inspector;
    DebugOverlayWorldState* world;
    DebugOverlayHistoryState* history;
} DebugOverlay;

void debug_overlay_history_push(DebugOverlay* overlay, const DebugOverlaySnapshot* snapshot, const EngineStatsSnapshot* stats);
float debug_overlay_history_sample(const float* history, size_t history_count, size_t history_cursor, size_t index);
float debug_overlay_compute_one_percent_low_fps(const DebugOverlay* overlay);
void debug_overlay_update_display_values(
    DebugOverlay* overlay,
    const DebugOverlaySnapshot* snapshot,
    float one_percent_low_fps
);
float debug_title_bar_height(void);
float debug_inspector_collapsed_width(void);
bool debug_point_in_rect(float x, float y, Rect rect);
Rect debug_panel_rect(const DebugPanelState* panel);
Rect debug_title_rect(const DebugPanelState* panel);
Rect debug_inspector_collapse_rect(const DebugPanelState* panel);
void debug_overlay_ensure_layout(
    DebugOverlay* overlay,
    bool has_selection,
    int viewport_width,
    int viewport_height
);
double debug_overlay_now_ms(void);
uint64_t debug_hash_u64(uint64_t hash, uint64_t value);
uint64_t debug_hash_i32(uint64_t hash, int32_t value);
int32_t debug_round_tenths(float value);
int32_t debug_round_whole(float value);
int32_t debug_round_quarters(float value);
void debug_draw_panel_frame(
    Target* target,
    const DebugPanelState* panel,
    const char* title,
    bool hovered
);
void debug_draw_inspector_rail(Target* target, const DebugPanelState* panel, bool hovered);
void debug_overlay_draw_dashboard_contents(
    DebugOverlay* overlay,
    Target* target,
    const DebugOverlaySnapshot* snapshot,
    const EngineStatsSnapshot* stats,
    int viewport_width,
    int viewport_height
);
void debug_overlay_draw_inspector_contents(
    DebugOverlay* overlay,
    Target* target,
    const DebugEntityView* selected_entity
);

#endif
