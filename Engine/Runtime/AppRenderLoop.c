#include "AppRenderLoop.h"

#include "InteractionSystem.h"
#include "../Rendering/DebugOverlay.h"
#include "../Rendering/RaylibBackend.h"
#include "../Rendering/Renderer.h"

#include <time.h>

static void engine_app_render_sleep_ms(uint32_t milliseconds)
{
    struct timespec request;

    request.tv_sec = (time_t)(milliseconds / 1000U);
    request.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    nanosleep(&request, NULL);
}

static const RenderSnapshotBuffer* engine_app_acquire_render_snapshot(
    EngineAppContext* app,
    RenderSnapshotExchange* exchange,
    uint64_t* last_render_snapshot_sequence,
    const RenderSnapshotBuffer** retained_render_snapshot
)
{
    const RenderSnapshotBuffer* next_render_snapshot;

    if (app == NULL || exchange == NULL || last_render_snapshot_sequence == NULL || retained_render_snapshot == NULL)
    {
        return NULL;
    }

    next_render_snapshot = render_snapshot_exchange_acquire_if_new(exchange, *last_render_snapshot_sequence);
    if (next_render_snapshot == NULL)
    {
        return *retained_render_snapshot;
    }

    if (*retained_render_snapshot != NULL)
    {
        render_snapshot_exchange_release_published(exchange, *retained_render_snapshot);
    }
    if (*last_render_snapshot_sequence > 0U &&
        next_render_snapshot->sequence > *last_render_snapshot_sequence + 1U)
    {
        app->dropped_render_snapshots += next_render_snapshot->sequence - *last_render_snapshot_sequence - 1U;
    }

    *last_render_snapshot_sequence = next_render_snapshot->sequence;
    *retained_render_snapshot = next_render_snapshot;
    return next_render_snapshot;
}

void engine_app_run_render_loop(
    EngineAppContext* app,
    const EngineSettings* settings,
    RenderSnapshotExchange* render_snapshot_exchange,
    InputPacketStream* input_stream
)
{
    uint64_t last_render_snapshot_sequence = 0U;
    const RenderSnapshotBuffer* retained_render_snapshot = NULL;

    if (app == NULL || settings == NULL || render_snapshot_exchange == NULL || input_stream == NULL)
    {
        return;
    }

    while (!raylib_backend_should_close(&app->backend))
    {
        const RenderSnapshotBuffer* render_snapshot;
        EngineRuntimeInputPacket* next_input_packet;
        DebugOverlaySnapshot debug_overlay_snapshot;
        EngineStatsSnapshot engine_stats_snapshot;
        EngineInputSnapshot input_snapshot;
        RendererFrame frame;
        const DebugEntityView* selected_entity;
        bool has_selection;
        bool pointer_over_ui;

        raylib_backend_pump_events(&app->backend);
        input_snapshot = raylib_backend_snapshot_input(&app->backend);
        Engine_update(&app->engine, 0.0, &input_snapshot);
        if (EngineInput_was_pressed(&app->engine.input, ENGINE_INPUT_ACTION_DEBUG_TOGGLE))
        {
            debug_overlay_toggle(renderer_get_debug_overlay(app->renderer));
        }
        if (EngineInput_was_pressed(&app->engine.input, ENGINE_INPUT_ACTION_DEBUG_WORLD_TOGGLE))
        {
            debug_overlay_toggle_world_gizmos(renderer_get_debug_overlay(app->renderer));
        }

        engine_stats_snapshot = Engine_get_stats_snapshot(&app->engine);
        render_snapshot = engine_app_acquire_render_snapshot(
            app,
            render_snapshot_exchange,
            &last_render_snapshot_sequence,
            &retained_render_snapshot
        );
        if (render_snapshot == NULL)
        {
            engine_app_render_sleep_ms(settings->app_idle_sleep_ms);
            continue;
        }

        selected_entity = render_snapshot->world.has_selected_entity ? &render_snapshot->world.selected_entity : NULL;
        has_selection = selected_entity != NULL || app->interaction_state.selected_entity_id != 0U;
        debug_overlay_handle_input(
            renderer_get_debug_overlay(app->renderer),
            &app->engine.input,
            has_selection,
            app->backend.window_width,
            app->backend.window_height
        );
        pointer_over_ui = debug_overlay_is_pointer_over_ui(
            renderer_get_debug_overlay(app->renderer),
            has_selection,
            (float)input_snapshot.mouse_x,
            (float)input_snapshot.mouse_y,
            app->backend.window_width,
            app->backend.window_height
        );
        InteractionSystem_UpdateRender(
            &app->interaction_state,
            NULL,
            app->renderer,
            &app->engine.input,
            render_snapshot,
            pointer_over_ui,
            engine_stats_snapshot.frame_ms > 0.0 ? engine_stats_snapshot.frame_ms / 1000.0 : 0.0
        );

        next_input_packet = input_packet_stream_begin_write(input_stream);
        if (next_input_packet != NULL)
        {
            InteractionSystem_WriteInputPacket(
                &app->interaction_state,
                &input_snapshot,
                app->renderer,
                pointer_over_ui,
                debug_overlay_is_ui_visible(renderer_get_debug_overlay(app->renderer)),
                debug_overlay_is_world_gizmos_visible(renderer_get_debug_overlay(app->renderer)),
                app->backend.window_width,
                app->backend.window_height,
                input_packet_stream_next_frame_id(input_stream),
                next_input_packet
            );
            input_packet_stream_publish(input_stream, next_input_packet, next_input_packet->frame_id);
        }
        InteractionSystem_ClearReleasedDrag(&app->interaction_state, &app->engine.input);

        renderer_drain_resource_commands(app->renderer, app->resource_command_queue);
        render_world_snapshot_build_frame(&render_snapshot->world, &frame);
        selected_entity = render_snapshot->world.has_selected_entity ? &render_snapshot->world.selected_entity : NULL;
        debug_overlay_snapshot = render_snapshot->stats.overlay;
        debug_overlay_snapshot.fps = (float)engine_stats_snapshot.fps;
        debug_overlay_snapshot.draw_ms = (float)engine_stats_snapshot.draw_ms;
        debug_overlay_snapshot.snapshot_age_ms =
            render_snapshot->published_at_ms > 0U && frame.now_ms >= render_snapshot->published_at_ms
                ? (float)(frame.now_ms - render_snapshot->published_at_ms)
                : 0.0f;

        Engine_draw_begin(&app->engine);
        raylib_backend_begin_frame(&app->backend, settings->app_clear_color);
        frame.selected_debug_entity_id = selected_entity != NULL ? selected_entity->id : app->interaction_state.selected_entity_id;
        frame.hovered_debug_entity_id = app->interaction_state.hovered_entity_id;
        renderer_draw(app->renderer, &app->backend.render_backend, &frame);
        debug_overlay_draw_ui(
            renderer_get_debug_overlay(app->renderer),
            &app->backend.render_backend,
            &debug_overlay_snapshot,
            &engine_stats_snapshot,
            selected_entity,
            app->backend.window_width,
            app->backend.window_height
        );
        raylib_backend_end_frame(&app->backend);
        Engine_draw_end(&app->engine);
    }

    if (retained_render_snapshot != NULL)
    {
        render_snapshot_exchange_release_published(render_snapshot_exchange, retained_render_snapshot);
    }
}
