#include "DebugOverlayInternal.h"

#include "DebugOverlayDashboardInternal.h"
#include "DebugOverlayHistoryInternal.h"
#include "DebugOverlayInspectorInternal.h"
#include "DebugOverlayUiInternal.h"
#include "DebugOverlayWorldInternal.h"
#include "../Core/Profiling.h"
#include "../Settings.h"

#include "../Core/Memory.h"

#include <stdlib.h>
#include <string.h>

static uint64_t debug_history_push_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_history_push_interval_ms;
}

static uint64_t debug_dashboard_redraw_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_dashboard_redraw_interval_ms;
}

static uint64_t debug_inspector_redraw_interval_ms(void) {
    return EngineSettings_GetDefaults()->debug_overlay_inspector_redraw_interval_ms;
}

void debug_overlay_init(DebugOverlay *overlay) {
    if (overlay == NULL) {
        return;
    }
    memset(overlay, 0, sizeof(*overlay));
    overlay->ui = (DebugOverlayUiState*)calloc(1U, sizeof(*overlay->ui));
    overlay->dashboard = (DebugOverlayDashboardState*)calloc(1U, sizeof(*overlay->dashboard));
    overlay->inspector = (DebugOverlayInspectorState*)calloc(1U, sizeof(*overlay->inspector));
    overlay->world = (DebugOverlayWorldState*)calloc(1U, sizeof(*overlay->world));
    overlay->history = (DebugOverlayHistoryState*)calloc(1U, sizeof(*overlay->history));
    if (overlay->ui == NULL ||
        overlay->dashboard == NULL ||
        overlay->inspector == NULL ||
        overlay->world == NULL ||
        overlay->history == NULL) {
        free(overlay->ui);
        free(overlay->dashboard);
        free(overlay->inspector);
        free(overlay->world);
        free(overlay->history);
        memset(overlay, 0, sizeof(*overlay));
        return;
    }
    overlay->ui->draw_ui = true;
    overlay->ui->draw_world_gizmos = false;
    overlay->ui->draw_ui_bounds = true;
    overlay->world->last_visible_entity_count = 0U;
    overlay->world->last_active_collision_count = 0U;
}

DebugOverlay* debug_overlay_create(void) {
    DebugOverlay* overlay = (DebugOverlay*)calloc(1U, sizeof(*overlay));

    if (overlay == NULL) {
        return NULL;
    }
    debug_overlay_init(overlay);
    if (overlay->ui == NULL ||
        overlay->dashboard == NULL ||
        overlay->inspector == NULL ||
        overlay->world == NULL ||
        overlay->history == NULL) {
        free(overlay);
        return NULL;
    }
    return overlay;
}

void debug_overlay_dispose(DebugOverlay *overlay) {
    if (overlay == NULL) {
        return;
    }
    if (overlay->dashboard != NULL &&
        overlay->dashboard->texture != NULL &&
        overlay->dashboard->texture_backend != NULL &&
        overlay->dashboard->texture_backend->destroy_texture != NULL) {
        overlay->dashboard->texture_backend->destroy_texture(
            overlay->dashboard->texture_backend->userdata,
            overlay->dashboard->texture
        );
    }
    if (overlay->inspector != NULL &&
        overlay->inspector->texture != NULL &&
        overlay->inspector->texture_backend != NULL &&
        overlay->inspector->texture_backend->destroy_texture != NULL) {
        overlay->inspector->texture_backend->destroy_texture(
            overlay->inspector->texture_backend->userdata,
            overlay->inspector->texture
        );
    }
    if (overlay->world != NULL &&
        overlay->world->texture != NULL &&
        overlay->world->texture_backend != NULL &&
        overlay->world->texture_backend->destroy_texture != NULL) {
        overlay->world->texture_backend->destroy_texture(
            overlay->world->texture_backend->userdata,
            overlay->world->texture
        );
    }
    free(overlay->ui);
    free(overlay->dashboard);
    free(overlay->inspector);
    free(overlay->world);
    free(overlay->history);
    memset(overlay, 0, sizeof(*overlay));
}

void debug_overlay_destroy(DebugOverlay* overlay) {
    if (overlay == NULL) {
        return;
    }
    debug_overlay_dispose(overlay);
    free(overlay);
}

static void debug_overlay_tick_dashboard_state(
    DebugOverlay* overlay,
    const DebugOverlaySnapshot* snapshot,
    const EngineStatsSnapshot* stats
) {
    ENGINE_PROFILE_ZONE_BEGIN(history_zone, "debug_overlay_tick_dashboard_state");
    bool pushed_history = false;
    uint64_t now_ms = 0U;

    if (overlay == NULL || snapshot == NULL || stats == NULL) {
        ENGINE_PROFILE_ZONE_END(history_zone);
        return;
    }

    now_ms = (uint64_t)debug_overlay_now_ms();
    if (overlay->history->last_history_push_ms == 0U || stats->frame_ms <= 0.0 ||
        overlay->history->last_history_push_ms + debug_history_push_interval_ms() <= now_ms) {
        debug_overlay_history_push(overlay, snapshot, stats);
        overlay->history->last_history_push_ms = now_ms;
        pushed_history = true;
    }

    if (pushed_history || overlay->history->last_one_percent_low_fps <= 0.0f) {
        overlay->history->last_one_percent_low_fps = debug_overlay_compute_one_percent_low_fps(overlay);
    }

    debug_overlay_update_display_values(overlay, snapshot, overlay->history->last_one_percent_low_fps);
    ENGINE_PROFILE_ZONE_END(history_zone);
}

static bool debug_overlay_should_redraw_panel(
    uint64_t last_signature,
    uint64_t next_signature,
    uint64_t last_redraw_at_ms,
    uint64_t now_ms,
    uint64_t min_redraw_interval_ms,
    bool is_interacting
) {
    if (last_signature == 0ULL || last_signature != next_signature) {
        if (last_signature == 0ULL || is_interacting) {
            return true;
        }
        return last_redraw_at_ms == 0U ||
            now_ms >= last_redraw_at_ms + min_redraw_interval_ms;
    }

    return false;
}

static uint64_t debug_overlay_compute_dashboard_signature(
    const DebugOverlay* overlay,
    const DebugOverlaySnapshot* snapshot,
    int viewport_width,
    int viewport_height
) {
    uint64_t hash = 1469598103934665603ULL;

    if (overlay == NULL) {
        return 0ULL;
    }

    hash = debug_hash_i32(hash, viewport_width);
    hash = debug_hash_i32(hash, viewport_height);
    hash = debug_hash_i32(hash, debug_round_whole(overlay->history->display_fps));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->history->display_render_ms));
    hash = debug_hash_i32(hash, debug_round_whole(overlay->history->display_one_percent_low_fps));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->history->display_sim_ms));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->history->display_update_ms));
    hash = debug_hash_i32(hash, debug_round_quarters(overlay->history->display_physics_ms));
    hash = debug_hash_i32(hash, debug_round_whole(overlay->history->display_physics_hz));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->history->display_visible_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->history->display_awake_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->history->display_total_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->history->display_sleeping_body_count / 16.0f));
    hash = debug_hash_u64(hash, (uint64_t)(overlay->history->display_moved_body_count / 16.0f));
    if (snapshot != NULL) {
        hash = debug_hash_u64(hash, snapshot->particle_system_count);
        hash = debug_hash_u64(hash, snapshot->particle_count);
        hash = debug_hash_u64(hash, snapshot->particle_contact_count);
        hash = debug_hash_u64(hash, snapshot->particle_body_contact_count);
        hash = debug_hash_u64(hash, snapshot->particle_task_count);
        hash = debug_hash_u64(hash, snapshot->particle_task_range_count);
        hash = debug_hash_u64(hash, snapshot->particle_spatial_cell_count);
        hash = debug_hash_u64(hash, snapshot->particle_occupied_cell_count);
        hash = debug_hash_u64(hash, snapshot->particle_byte_count / 1024U);
        hash = debug_hash_u64(hash, snapshot->particle_scratch_byte_count / 1024U);
        hash = debug_hash_i32(hash, debug_round_quarters(snapshot->particle_profile_ms));
        hash = debug_hash_i32(hash, debug_round_quarters(snapshot->particle_profile_collision_ms));
    }
    hash = debug_hash_u64(hash, overlay->history->history_serial);
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->ui->dashboard_panel.x));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->ui->dashboard_panel.y));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->ui->dashboard_panel.width));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->ui->dashboard_panel.height));
    hash = debug_hash_u64(hash, overlay->ui->hovered_ui_region == DEBUG_UI_HOVER_DASHBOARD ? 1U : 0U);
    return hash;
}

static uint64_t debug_overlay_compute_inspector_signature(
    const DebugOverlay* overlay,
    const DebugEntityView* selected_entity,
    int viewport_width,
    int viewport_height
) {
    uint64_t hash = 1469598103934665603ULL;

    hash = debug_hash_i32(hash, viewport_width);
    hash = debug_hash_i32(hash, viewport_height);
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->ui->inspector_panel.x));
    hash = debug_hash_i32(hash, debug_round_tenths(overlay->ui->inspector_panel.y));
    hash = debug_hash_u64(hash, overlay->ui->inspector_collapsed ? 1U : 0U);
    hash = debug_hash_u64(hash, overlay->ui->hovered_ui_region == DEBUG_UI_HOVER_INSPECTOR ? 1U : 0U);
    if (selected_entity != NULL) {
        hash = debug_hash_u64(hash, selected_entity->id);
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->position.x));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->position.y));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->velocity.x));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->velocity.y));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->angle_radians));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->extent_x));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->extent_y));
        hash = debug_hash_i32(hash, debug_round_tenths(selected_entity->radius));
        hash = debug_hash_u64(hash, selected_entity->is_circle ? 1U : 0U);
        hash = debug_hash_u64(hash, selected_entity->is_selected ? 1U : 0U);
        hash = debug_hash_u64(hash, selected_entity->is_sleeping ? 1U : 0U);
        hash = debug_hash_u64(hash, selected_entity->is_moving ? 1U : 0U);
    }

    return hash;
}

void debug_overlay_draw_ui(
    DebugOverlay *overlay,
    RenderBackend *backend,
    const DebugOverlaySnapshot *snapshot,
    const EngineStatsSnapshot *stats,
    const DebugEntityView *selected_entity,
    int viewport_width,
    int viewport_height
) {
    ENGINE_PROFILE_ZONE_BEGIN(draw_ui_zone, "debug_overlay_draw_ui");
    Target target;
    bool has_selection;
    uint64_t dashboard_signature;
    uint64_t inspector_signature;
    uint64_t now_ms;
    double layout_started_ms = 0.0;
    double composite_started_ms = 0.0;

    if (overlay == NULL || !overlay->ui->draw_ui || backend == NULL || snapshot == NULL || stats == NULL) {
        ENGINE_PROFILE_ZONE_END(draw_ui_zone);
        return;
    }

    has_selection = selected_entity != NULL;
    now_ms = (uint64_t)debug_overlay_now_ms();
    layout_started_ms = debug_overlay_now_ms();
    ENGINE_PROFILE_ZONE_BEGIN(layout_zone, "debug_overlay_layout");
    debug_overlay_ensure_layout(overlay, has_selection, viewport_width, viewport_height);
    ENGINE_PROFILE_ZONE_END(layout_zone);
    overlay->ui->last_ui_layout_ms = debug_overlay_now_ms() - layout_started_ms;
    debug_overlay_tick_dashboard_state(overlay, snapshot, stats);
    dashboard_signature = debug_overlay_compute_dashboard_signature(overlay, snapshot, viewport_width, viewport_height);
    inspector_signature = debug_overlay_compute_inspector_signature(overlay, selected_entity, viewport_width, viewport_height);
    if (backend->create_texture != NULL &&
        backend->destroy_texture != NULL &&
        backend->set_target != NULL &&
        backend->reset_target != NULL &&
        backend->clear != NULL) {
        if (overlay->dashboard->texture == NULL ||
            overlay->dashboard->texture_width != (int)overlay->ui->dashboard_panel.width ||
            overlay->dashboard->texture_height != (int)overlay->ui->dashboard_panel.height) {
            if (overlay->dashboard->texture != NULL) {
                overlay->dashboard->texture_backend->destroy_texture(overlay->dashboard->texture_backend->userdata, overlay->dashboard->texture);
            }
            overlay->dashboard->texture = backend->create_texture(
                backend->userdata,
                (int)overlay->ui->dashboard_panel.width,
                (int)overlay->ui->dashboard_panel.height
            );
            overlay->dashboard->texture_backend = backend;
            overlay->dashboard->texture_width = (int)overlay->ui->dashboard_panel.width;
            overlay->dashboard->texture_height = (int)overlay->ui->dashboard_panel.height;
            overlay->dashboard->last_signature = 0ULL;
        }

        if (overlay->inspector->texture == NULL ||
            overlay->inspector->texture_width != (int)(overlay->ui->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->ui->inspector_panel.width) ||
            overlay->inspector->texture_height != (int)overlay->ui->inspector_panel.height) {
            if (overlay->inspector->texture != NULL) {
                overlay->inspector->texture_backend->destroy_texture(overlay->inspector->texture_backend->userdata, overlay->inspector->texture);
            }
            overlay->inspector->texture = backend->create_texture(
                backend->userdata,
                (int)(overlay->ui->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->ui->inspector_panel.width),
                (int)overlay->ui->inspector_panel.height
            );
            overlay->inspector->texture_backend = backend;
            overlay->inspector->texture_width = (int)(overlay->ui->inspector_collapsed ? debug_inspector_collapsed_width() : overlay->ui->inspector_panel.width);
            overlay->inspector->texture_height = (int)overlay->ui->inspector_panel.height;
            overlay->inspector->last_signature = 0ULL;
        }

        overlay->ui->last_ui_draw_ms = 0.0;
        if (overlay->dashboard->texture != NULL && debug_overlay_should_redraw_panel(
                overlay->dashboard->last_signature,
                dashboard_signature,
                overlay->dashboard->last_redraw_at_ms,
                now_ms,
                debug_dashboard_redraw_interval_ms(),
                overlay->ui->dashboard_panel.dragging)) {
            ENGINE_PROFILE_ZONE_BEGIN(dashboard_zone, "debug_overlay_dashboard_redraw");
            double redraw_started_ms = debug_overlay_now_ms();
            backend->set_target(backend->userdata, overlay->dashboard->texture);
            backend->clear(backend->userdata, (Color32){ 0x00000000U });
            target_init(&target, backend, -overlay->ui->dashboard_panel.x, -overlay->ui->dashboard_panel.y);
            debug_overlay_draw_dashboard_contents(overlay, &target, snapshot, stats, viewport_width, viewport_height);
            backend->reset_target(backend->userdata);
            overlay->dashboard->last_signature = dashboard_signature;
            overlay->dashboard->last_redraw_at_ms = now_ms;
            overlay->dashboard->redraw_count += 1U;
            overlay->dashboard->last_redraw_ms = debug_overlay_now_ms() - redraw_started_ms;
            overlay->ui->last_ui_draw_ms += overlay->dashboard->last_redraw_ms;
            ENGINE_PROFILE_ZONE_END(dashboard_zone);
        } else {
            overlay->dashboard->redraw_skip_count += 1U;
        }

        if (overlay->inspector->texture != NULL && debug_overlay_should_redraw_panel(
                overlay->inspector->last_signature,
                inspector_signature,
                overlay->inspector->last_redraw_at_ms,
                now_ms,
                debug_inspector_redraw_interval_ms(),
                overlay->ui->inspector_panel.dragging)) {
            ENGINE_PROFILE_ZONE_BEGIN(inspector_zone, "debug_overlay_inspector_redraw");
            double redraw_started_ms = debug_overlay_now_ms();
            backend->set_target(backend->userdata, overlay->inspector->texture);
            backend->clear(backend->userdata, (Color32){ 0x00000000U });
            target_init(&target, backend, -overlay->ui->inspector_panel.x, -overlay->ui->inspector_panel.y);
            if (selected_entity != NULL) {
                debug_overlay_draw_inspector_contents(overlay, &target, selected_entity);
            }
            backend->reset_target(backend->userdata);
            overlay->inspector->last_signature = inspector_signature;
            overlay->inspector->last_redraw_at_ms = now_ms;
            overlay->inspector->redraw_count += 1U;
            overlay->inspector->last_redraw_ms = debug_overlay_now_ms() - redraw_started_ms;
            overlay->ui->last_ui_draw_ms += overlay->inspector->last_redraw_ms;
            ENGINE_PROFILE_ZONE_END(inspector_zone);
        } else {
            overlay->inspector->redraw_skip_count += 1U;
        }

        if (overlay->dashboard->texture != NULL) {
            ENGINE_PROFILE_ZONE_BEGIN(composite_zone, "debug_overlay_composite");
            composite_started_ms = debug_overlay_now_ms();
            target_init(&target, backend, 0.0f, 0.0f);
            target_texture(&target, overlay->dashboard->texture, overlay->ui->dashboard_panel.x, overlay->ui->dashboard_panel.y);
            if (selected_entity != NULL && overlay->inspector->texture != NULL) {
                target_texture(&target, overlay->inspector->texture, overlay->ui->inspector_panel.x, overlay->ui->inspector_panel.y);
            }
            overlay->ui->last_ui_composite_ms = debug_overlay_now_ms() - composite_started_ms;
            ENGINE_PROFILE_ZONE_END(composite_zone);
            ENGINE_PROFILE_ZONE_END(draw_ui_zone);
            return;
        }
    }

    target_init(&target, backend, 0.0f, 0.0f);
    debug_overlay_draw_dashboard_contents(overlay, &target, snapshot, stats, viewport_width, viewport_height);
    if (selected_entity != NULL) {
        debug_overlay_draw_inspector_contents(overlay, &target, selected_entity);
    }
    ENGINE_PROFILE_ZONE_END(draw_ui_zone);
}
