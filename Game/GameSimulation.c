#include "Game/GameInternal.h"

#include <math.h>
#include <stdatomic.h>
#include <string.h>

void* game_simulation_thread_main(void* user_data) {
    SimThreadContext* context = (SimThreadContext*)user_data;
    GameState* game;
    EngineInput sim_input;
    EngineInputBindings bindings;
    EngineInputSnapshot empty_snapshot;
    GameInputPacket latest_input;
    uint64_t last_tick_ms;
    uint64_t last_snapshot_publish_ms;
    uint64_t last_input_frame_id = 0U;
    bool has_input = false;

    if (context == NULL || context->game == NULL || context->render_snapshot_exchange == NULL || context->input_stream == NULL) {
        return NULL;
    }

    game = context->game;
    EngineInputBindings_set_defaults(&bindings);
    EngineInput_init(&sim_input, &bindings);
    memset(&empty_snapshot, 0, sizeof(empty_snapshot));
    memset(&latest_input, 0, sizeof(latest_input));
    last_tick_ms = (uint64_t)game_now_ms();
    last_snapshot_publish_ms = last_tick_ms;

    while (!atomic_load_explicit(&context->shutdown_requested, memory_order_acquire)) {
        GameInputPacket packet;
        RenderSnapshotBuffer* next_render_snapshot;
        PhysicsWorldSnapshot physics_snapshot;
        uint64_t now_ms;
        double dt_seconds;
        double update_started_ms;
        double update_ms;
        double fixed_step_started_ms;
        double fixed_step_wall_ms;
        double input_started_ms;
        double input_ms;
        double spawn_started_ms;
        double spawn_ms;
        double drag_started_ms;
        double drag_ms;
        double snapshot_acquire_started_ms;
        double snapshot_acquire_ms;
        double snapshot_build_started_ms;
        double snapshot_build_ms;
        double fixed_dt_for_step;
        uint32_t max_substeps_for_step;
        bool step_expected;
        bool input_captured = false;
        bool has_drag_packet = false;
        bool snapshot_publish_due = false;
        GameCullProfile cull_profile;
        uint32_t physics_substeps;
        float render_alpha = 1.0f;
        Aabb snapshot_view_bounds;
        bool input_packet_read = false;
        bool input_changed = false;

        if (input_packet_stream_read_latest(context->input_stream, &packet)) {
            latest_input = packet;
            has_input = true;
            input_packet_read = true;
            input_changed = packet.frame_id != last_input_frame_id;
            last_input_frame_id = packet.frame_id;
        }
        if (input_packet_read) {
            game->cached_debug_overlay_enabled = latest_input.debug_overlay_enabled;
            game->cached_draw_debug_world = latest_input.draw_debug_world;
        }

        now_ms = (uint64_t)game_now_ms();
        dt_seconds = (double)(now_ms - last_tick_ms) / 1000.0;
        if (dt_seconds < 0.0) {
            dt_seconds = 0.0;
        } else if (dt_seconds > 0.1) {
            dt_seconds = 0.1;
        }
        last_tick_ms = now_ms;
        update_started_ms = game_now_ms();
        fixed_dt_for_step = game->cached_sim_fixed_dt_seconds > 0.0
            ? game->cached_sim_fixed_dt_seconds
            : game->sim_step_dt_seconds;
        fixed_dt_for_step = fixed_dt_for_step > 0.0 ? fixed_dt_for_step : 1.0 / 60.0;
        max_substeps_for_step = game->cached_sim_max_substeps > 0U ? game->cached_sim_max_substeps : 4U;
        step_expected =
            fmin(game->accumulator_seconds + dt_seconds, fixed_dt_for_step * (double)max_substeps_for_step) >=
            fixed_dt_for_step;

        input_started_ms = game_now_ms();
        if (input_changed || step_expected) {
            EngineInput_capture(&sim_input, has_input ? &latest_input.snapshot : &empty_snapshot);
            input_captured = true;
            if (EngineInput_was_pressed(&sim_input, ENGINE_INPUT_ACTION_CYCLE_TARGET)) {
                game->tracked_ball_index = (game->tracked_ball_index + 1U) % BALL_COUNT;
            }
        }
        input_ms = game_now_ms() - input_started_ms;

        spawn_started_ms = game_now_ms();
        game_update_spawn_curve(game, dt_seconds);
        spawn_ms = game_now_ms() - spawn_started_ms;

        fixed_step_started_ms = game_now_ms();
        physics_substeps = game_step_world(game, dt_seconds, &sim_input);
        fixed_step_wall_ms = game_now_ms() - fixed_step_started_ms;
        memset(&physics_snapshot, 0, sizeof(physics_snapshot));
        drag_started_ms = game_now_ms();
        has_drag_packet = has_input && (latest_input.drag_entity_active || latest_input.drag_entity_release);
        if (has_drag_packet && (input_changed || latest_input.drag_entity_active || latest_input.drag_entity_release)) {
            game_apply_entity_drag_packet(game, &latest_input);
        }
        drag_ms = game_now_ms() - drag_started_ms;
        snapshot_publish_due =
            input_changed ||
            game->entity_drag_active ||
            game->camera_pan_active ||
            now_ms - last_snapshot_publish_ms >= SIM_SNAPSHOT_PUBLISH_INTERVAL_MS;
        if (!snapshot_publish_due) {
            if (physics_substeps == 0U) {
                game_sleep_ms(SIM_IDLE_SLEEP_MS);
            }
            continue;
        }
        render_alpha = game_compute_render_alpha(game);
        snapshot_acquire_started_ms = game_now_ms();
        next_render_snapshot = render_snapshot_exchange_begin_write(context->render_snapshot_exchange);
        snapshot_acquire_ms = game_now_ms() - snapshot_acquire_started_ms;
        if (next_render_snapshot == NULL) {
            game_sleep_ms(SIM_IDLE_SLEEP_MS);
            continue;
        }
        if (game->scene != NULL && Scene_GetPhysicsWorld(game->scene) != NULL) {
            PhysicsWorld_GetSnapshot(Scene_GetPhysicsWorld(game->scene), &physics_snapshot);
            if (physics_substeps == 0U) {
                physics_snapshot.profile_step_ms = 0.0f;
                physics_snapshot.profile_pairs_ms = 0.0f;
                physics_snapshot.profile_collide_ms = 0.0f;
                physics_snapshot.profile_solve_ms = 0.0f;
            }
        }
        if (game->engine.profiler.enabled) {
            game->last_player_neighborhood_count = (uint32_t)game_query_player_neighborhood(game, 180.0f);
        } else {
            game->last_player_neighborhood_count = 0U;
        }
        snapshot_view_bounds = game_get_cached_snapshot_view_bounds(game, &latest_input, has_input);
        snapshot_build_started_ms = game_now_ms();
        game_build_render_snapshot(
            game,
            next_render_snapshot,
            render_alpha,
            now_ms,
            0.0,
            physics_substeps,
            game->cached_debug_overlay_enabled,
            game->cached_draw_debug_world,
            snapshot_view_bounds,
            &cull_profile,
            &physics_snapshot
        );
        snapshot_build_ms = game_now_ms() - snapshot_build_started_ms;
        update_ms = game_now_ms() - update_started_ms;
        next_render_snapshot->stats.overlay.update_ms = (float)fmax(update_ms - fixed_step_wall_ms, 0.0);
        next_render_snapshot->stats.overlay.sim_ms = (float)update_ms;
        next_render_snapshot->stats.sim_input_ms = input_captured ? (float)input_ms : 0.0f;
        next_render_snapshot->stats.sim_spawn_ms = (float)spawn_ms;
        next_render_snapshot->stats.sim_fixed_step_wall_ms = (float)fixed_step_wall_ms;
        next_render_snapshot->stats.sim_drag_ms = (float)drag_ms;
        next_render_snapshot->stats.sim_snapshot_acquire_ms = (float)snapshot_acquire_ms;
        next_render_snapshot->stats.sim_snapshot_build_ms = (float)snapshot_build_ms;
        if (context->resource_command_queue != NULL) {
            next_render_snapshot->stats.resource_commands_enqueued =
                resource_command_queue_get_enqueued_count(context->resource_command_queue);
            next_render_snapshot->stats.resource_commands_dropped =
                resource_command_queue_get_dropped_count(context->resource_command_queue);
        }
        render_snapshot_exchange_publish(context->render_snapshot_exchange, next_render_snapshot);
        last_snapshot_publish_ms = now_ms;
    }

    return NULL;
}
