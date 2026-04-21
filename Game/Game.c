#include "Game/Game.h"
#include "Game/GameInternal.h"

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int JimokomiGame_Run(void) {
    GameState game;
    RuntimeConfig runtime_config;
    RendererConfig renderer_config;
    TaskSystemConfig task_system_config;
    InputPacketStream input_stream;
    RenderSnapshotExchange render_snapshot_exchange;
    ResourceCommandQueue* resource_command_queue;
    SimThreadContext sim_context;
    pthread_t sim_thread;
    uint64_t last_render_snapshot_sequence = 0U;
    uint64_t dropped_render_snapshots = 0U;
    const RenderSnapshotBuffer* retained_render_snapshot = NULL;
    char log_message[128];

    memset(&game, 0, sizeof(game));
    memset(&input_stream, 0, sizeof(input_stream));
    memset(&render_snapshot_exchange, 0, sizeof(render_snapshot_exchange));
    resource_command_queue = NULL;
    memset(&sim_context, 0, sizeof(sim_context));
    runtime_config_init_defaults(&runtime_config);
    if (!input_packet_stream_init(&input_stream, sizeof(GameInputPacket))) {
        return 1;
    }
    game.tracked_ball_index = PLAYER_INDEX;
    game.selected_ball_index = INVALID_INDEX;
    game.hovered_ball_index = INVALID_INDEX;
    game.hover_cache_valid = false;
    game.dragged_ball_index = INVALID_INDEX;
    game.active_ball_count = 0U;
    game.spawn_cursor = 0U;
    if (!render_snapshot_exchange_init(&render_snapshot_exchange, BALL_COUNT, BALL_COUNT, 256U)) {
        input_packet_stream_dispose(&input_stream);
        return 1;
    }
    resource_command_queue = resource_command_queue_create(BALL_COUNT * 4U);
    if (resource_command_queue == NULL) {
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }
    if (!raylib_backend_init(
        &game.backend,
        runtime_config.window_width,
        runtime_config.window_height,
        runtime_config.window_title
    )) {
        resource_command_queue_destroy(resource_command_queue);
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }

    if (!Engine_init(&game.engine, &runtime_config.engine)) {
        raylib_backend_dispose(&game.backend);
        resource_command_queue_destroy(resource_command_queue);
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }

    memset(&task_system_config, 0, sizeof(task_system_config));
    task_system_config.requested_worker_count = 0;
    if (!task_system_init(&game.task_system, &task_system_config)) {
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        resource_command_queue_destroy(resource_command_queue);
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }
    snprintf(
        log_message,
        sizeof(log_message),
        "runtime initialized cores=%d physics=%d prebake_budget=%zu",
        task_system_get_online_core_count(&game.task_system),
        task_system_get_worker_count(&game.task_system),
        runtime_config.renderer.prebake_budget_per_frame
    );
    EngineLogger_info(&game.engine.logger, log_message, "physics");

    renderer_config = runtime_config.renderer;
    game.renderer = renderer_create(&game.backend.render_backend, &renderer_config);
    if (game.renderer == NULL) {
        task_system_dispose(&game.task_system);
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        resource_command_queue_destroy(resource_command_queue);
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }
    if (!game_register_ball_visuals(
            game.renderer,
            &runtime_config,
            &game.shared_ball_shader_handle,
            game.ball_source_handles,
            SOURCE_VARIANT_COUNT,
            game.ball_material_handles,
            BALL_COUNT)) {
        renderer_destroy(game.renderer);
        task_system_dispose(&game.task_system);
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        resource_command_queue_destroy(resource_command_queue);
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }
    for (size_t i = 0U; i < BALL_COUNT; ++i) {
        game.balls[i].visual_source_handle = game.ball_source_handles[i % SOURCE_VARIANT_COUNT];
        game.balls[i].material_handle = game.ball_material_handles[i];
    }
    game_init_world(&game);
    if (game.scene == NULL) {
        renderer_destroy(game.renderer);
        task_system_dispose(&game.task_system);
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        resource_command_queue_destroy(resource_command_queue);
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }
    {
        SceneView scene_view = Scene_GetViewSnapshot(game.scene);
        Camera* render_camera = renderer_get_camera(game.renderer);
        if (render_camera != NULL) {
            int viewport_width = 0;
            int viewport_height = 0;
            renderer_get_viewport_size(game.renderer, &viewport_width, &viewport_height);
            render_camera->x = scene_view.x;
            render_camera->y = scene_view.y;
            render_camera->view_width = WORLD_WIDTH;
            render_camera->view_height = WORLD_HEIGHT;
            render_camera->viewport_width = (float)viewport_width;
            render_camera->viewport_height = (float)viewport_height;
            camera_set_world_bounds(render_camera, (Rect){ 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT });
        }
    }
    sim_context.game = &game;
    sim_context.render_snapshot_exchange = &render_snapshot_exchange;
    sim_context.input_stream = &input_stream;
    sim_context.resource_command_queue = resource_command_queue;
    atomic_init(&sim_context.shutdown_requested, false);
    if (pthread_create(&sim_thread, NULL, game_simulation_thread_main, &sim_context) != 0) {
        Scene_Destroy(game.scene);
        renderer_destroy(game.renderer);
        task_system_dispose(&game.task_system);
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        resource_command_queue_destroy(resource_command_queue);
        render_snapshot_exchange_dispose(&render_snapshot_exchange);
        input_packet_stream_dispose(&input_stream);
        return 1;
    }

    while (!raylib_backend_should_close(&game.backend)) {
        const RenderSnapshotBuffer* render_snapshot;
        GameInputPacket* next_input_packet;
        DebugOverlaySnapshot debug_overlay_snapshot;
        EngineStatsSnapshot engine_stats_snapshot;
        size_t drained_resource_commands;
        EngineInputSnapshot input_snapshot;
        RendererFrame frame;
        const DebugEntityView *selected_entity;
        const DebugEntityView *hovered_entity;
        bool has_selection;
        bool pointer_over_ui;

        raylib_backend_pump_events(&game.backend);
        input_snapshot = raylib_backend_snapshot_input(&game.backend);
        Engine_update(&game.engine, 0.0, &input_snapshot);
        if (EngineInput_was_pressed(&game.engine.input, ENGINE_INPUT_ACTION_DEBUG_TOGGLE)) {
            debug_overlay_toggle(renderer_get_debug_overlay(game.renderer));
        }
        if (EngineInput_was_pressed(&game.engine.input, ENGINE_INPUT_ACTION_DEBUG_WORLD_TOGGLE)) {
            debug_overlay_toggle_world_gizmos(renderer_get_debug_overlay(game.renderer));
        }

        engine_stats_snapshot = Engine_get_stats_snapshot(&game.engine);
        {
            const RenderSnapshotBuffer* next_render_snapshot =
                render_snapshot_exchange_acquire_if_new(&render_snapshot_exchange, last_render_snapshot_sequence);
            if (next_render_snapshot != NULL) {
                if (retained_render_snapshot != NULL) {
                    render_snapshot_exchange_release_published(
                        &render_snapshot_exchange,
                        retained_render_snapshot
                    );
                }
                if (last_render_snapshot_sequence > 0U &&
                    next_render_snapshot->sequence > last_render_snapshot_sequence + 1U) {
                    dropped_render_snapshots +=
                        next_render_snapshot->sequence - last_render_snapshot_sequence - 1U;
                }
                last_render_snapshot_sequence = next_render_snapshot->sequence;
                retained_render_snapshot = next_render_snapshot;
            }
        }
        render_snapshot = retained_render_snapshot;
        selected_entity = NULL;
        hovered_entity = NULL;
        has_selection = game.selected_ball_index != INVALID_INDEX;
        if (render_snapshot == NULL) {
            game_sleep_ms(SIM_IDLE_SLEEP_MS);
            continue;
        }

        debug_overlay_handle_input(
            renderer_get_debug_overlay(game.renderer),
            &game.engine.input,
            has_selection,
            game.backend.window_width,
            game.backend.window_height
        );

        pointer_over_ui = debug_overlay_is_pointer_over_ui(
            renderer_get_debug_overlay(game.renderer),
            has_selection,
            (float)input_snapshot.mouse_x,
            (float)input_snapshot.mouse_y,
            game.backend.window_width,
            game.backend.window_height
        );

        game_update_render_interactions(
            &game,
            game.renderer,
            &game.engine.input,
            render_snapshot,
            pointer_over_ui,
            engine_stats_snapshot.frame_ms > 0.0 ? engine_stats_snapshot.frame_ms / 1000.0 : 0.0
        );
        selected_entity = render_snapshot->world.has_selected_entity
            ? &render_snapshot->world.selected_entity
            : NULL;
        hovered_entity = render_snapshot->world.has_hovered_entity
            ? &render_snapshot->world.hovered_entity
            : NULL;
        has_selection = selected_entity != NULL;

        next_input_packet = input_packet_stream_begin_write(&input_stream);
        if (next_input_packet != NULL) {
            next_input_packet->snapshot = input_snapshot;
            next_input_packet->debug_overlay_enabled =
                debug_overlay_is_ui_visible(renderer_get_debug_overlay(game.renderer));
            next_input_packet->draw_debug_world =
                debug_overlay_is_world_gizmos_visible(renderer_get_debug_overlay(game.renderer));
            next_input_packet->drag_entity_active = game.entity_drag_active && game.dragged_ball_index != INVALID_INDEX;
            next_input_packet->drag_entity_release = !game.entity_drag_active && game.dragged_ball_index != INVALID_INDEX &&
                EngineInput_was_mouse_released(&game.engine.input, 1U);
            next_input_packet->drag_ball_index = game.dragged_ball_index;
            next_input_packet->drag_world_x = game.drag_world_position.x;
            next_input_packet->drag_world_y = game.drag_world_position.y;
            next_input_packet->drag_linear_velocity_x = game.drag_release_velocity.x;
            next_input_packet->drag_linear_velocity_y = game.drag_release_velocity.y;
            {
                const Camera* render_camera = renderer_get_camera_const(game.renderer);
                if (render_camera != NULL) {
                    next_input_packet->camera_x = render_camera->x;
                    next_input_packet->camera_y = render_camera->y;
                    next_input_packet->camera_view_width = render_camera->view_width;
                    next_input_packet->camera_view_height = render_camera->view_height;
                }
            }
            next_input_packet->window_width = game.backend.window_width;
            next_input_packet->window_height = game.backend.window_height;
            next_input_packet->pointer_over_ui = pointer_over_ui;
            next_input_packet->frame_id = input_packet_stream_next_frame_id(&input_stream);
            input_packet_stream_publish(&input_stream, next_input_packet, next_input_packet->frame_id);
        }
        if (!game.entity_drag_active && EngineInput_was_mouse_released(&game.engine.input, 1U)) {
            game.dragged_ball_index = INVALID_INDEX;
            game.drag_release_velocity = (Vec2){ 0.0f, 0.0f };
        }

        drained_resource_commands = renderer_drain_resource_commands(game.renderer, resource_command_queue);

        render_world_snapshot_build_frame(&render_snapshot->world, &frame);
        debug_overlay_snapshot = render_snapshot->stats.overlay;
        debug_overlay_snapshot.fps = (float)engine_stats_snapshot.fps;
        debug_overlay_snapshot.draw_ms = (float)engine_stats_snapshot.draw_ms;
        debug_overlay_snapshot.snapshot_age_ms =
            render_snapshot->published_at_ms > 0U && frame.now_ms >= render_snapshot->published_at_ms
                ? (float)(frame.now_ms - render_snapshot->published_at_ms)
                : 0.0f;

        Engine_draw_begin(&game.engine);
        raylib_backend_begin_frame(&game.backend, (Color32){ 0x0b1020U });
        frame.selected_debug_entity_id = selected_entity != NULL ? selected_entity->id : 0U;
        frame.hovered_debug_entity_id = hovered_entity != NULL ? hovered_entity->id : 0U;
        EngineProfiler_begin_scope(&game.engine.profiler, "draw.renderer");
        renderer_draw(game.renderer, &game.backend.render_backend, &frame);
        EngineProfiler_end_scope(&game.engine.profiler, "draw.renderer");
        EngineProfiler_begin_scope(&game.engine.profiler, "draw.debug_ui");
        debug_overlay_draw_ui(
            renderer_get_debug_overlay(game.renderer),
            &game.backend.render_backend,
            &debug_overlay_snapshot,
            &engine_stats_snapshot,
            selected_entity,
            game.backend.window_width,
            game.backend.window_height
        );
        EngineProfiler_end_scope(&game.engine.profiler, "draw.debug_ui");
        game_emit_profiler_metadata(
            &game,
            render_snapshot,
            &frame,
            drained_resource_commands,
            dropped_render_snapshots,
            resource_command_queue
        );
        raylib_backend_end_frame(&game.backend);
        Engine_draw_end(&game.engine);
        game.spawn_region_dirty_count = 0U;
    }

    atomic_store_explicit(&sim_context.shutdown_requested, true, memory_order_release);
    pthread_join(sim_thread, NULL);
    if (retained_render_snapshot != NULL) {
        render_snapshot_exchange_release_published(&render_snapshot_exchange, retained_render_snapshot);
        retained_render_snapshot = NULL;
    }

    if (game.scene != NULL) {
        Scene_Destroy(game.scene);
    }
    task_system_dispose(&game.task_system);
    renderer_destroy(game.renderer);
    Engine_dispose(&game.engine);
    raylib_backend_dispose(&game.backend);
    resource_command_queue_destroy(resource_command_queue);
    render_snapshot_exchange_dispose(&render_snapshot_exchange);
    input_packet_stream_dispose(&input_stream);
    return 0;
}
