#include "AppSimulation.h"

#include "InteractionSystem.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/SceneRenderSnapshot.h"
#include "../Scene/Scene.h"
#include "../Scene/ScenePhysics.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double engine_app_sim_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void engine_app_sim_sleep_ms(uint32_t milliseconds)
{
    struct timespec request;

    request.tv_sec = (time_t)(milliseconds / 1000U);
    request.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    nanosleep(&request, NULL);
}

static void engine_app_sim_get_scene_step_config(EngineAppSimThreadContext* context)
{
    PhysicsWorld* physics_world;
    float fixed_dt = 0.0f;
    uint32_t max_substeps = 0U;

    if (context == NULL || context->app == NULL || context->app->scene == NULL)
    {
        return;
    }

    physics_world = Scene_GetPhysicsWorld(context->app->scene);
    if (physics_world == NULL)
    {
        return;
    }

    PhysicsWorld_GetStepConfig(physics_world, &fixed_dt, &max_substeps);
    context->fixed_dt_seconds = fixed_dt > 0.0f ? (double)fixed_dt : 1.0 / 60.0;
    context->max_substeps = max_substeps > 0U ? max_substeps : 4U;
}

static float engine_app_sim_compute_render_alpha(const EngineAppSimThreadContext* context)
{
    if (context == NULL || context->fixed_dt_seconds <= 0.0)
    {
        return 1.0f;
    }

    return clamp_f((float)(context->accumulator_seconds / context->fixed_dt_seconds), 0.0f, 1.0f);
}

static uint32_t engine_app_sim_step_scene(
    EngineAppSimThreadContext* context,
    double dt_seconds,
    const EngineInput* input
)
{
    SceneInputState scene_input = { 0 };
    double fixed_dt;
    uint32_t max_substeps;

    if (context == NULL || context->app == NULL || context->app->scene == NULL)
    {
        return 0U;
    }

    fixed_dt = context->fixed_dt_seconds > 0.0 ? context->fixed_dt_seconds : 1.0 / 60.0;
    max_substeps = context->max_substeps > 0U ? context->max_substeps : 4U;
    context->accumulator_seconds = fmin(
        context->accumulator_seconds + dt_seconds,
        fixed_dt * (double)max_substeps
    );
    if (context->accumulator_seconds < fixed_dt)
    {
        return 0U;
    }

    if (input != NULL)
    {
        scene_input.buttons[ENGINE_INPUT_ACTION_LEFT] = EngineInput_is_down(input, ENGINE_INPUT_ACTION_LEFT);
        scene_input.buttons[ENGINE_INPUT_ACTION_RIGHT] = EngineInput_is_down(input, ENGINE_INPUT_ACTION_RIGHT);
        scene_input.buttons[ENGINE_INPUT_ACTION_UP] = EngineInput_is_down(input, ENGINE_INPUT_ACTION_UP);
        scene_input.buttons[ENGINE_INPUT_ACTION_DOWN] = EngineInput_is_down(input, ENGINE_INPUT_ACTION_DOWN);
        scene_input.buttons[ENGINE_INPUT_ACTION_JUMP] = EngineInput_was_pressed(input, ENGINE_INPUT_ACTION_JUMP);
        scene_input.buttons[ENGINE_INPUT_ACTION_ACTION] = EngineInput_was_pressed(input, ENGINE_INPUT_ACTION_ACTION);
        scene_input.mouse_x = (float)input->mouse_x;
        scene_input.mouse_y = (float)input->mouse_y;
        scene_input.mouse_primary_down = EngineInput_is_mouse_down(input, 1U);
        scene_input.mouse_primary_pressed = EngineInput_was_mouse_pressed(input, 1U);
        scene_input.mouse_primary_released = EngineInput_was_mouse_released(input, 1U);
    }

    Scene_Update(context->app->scene, (float)fixed_dt, &scene_input);
    context->accumulator_seconds -= fixed_dt;
    return 1U;
}

void* engine_app_simulation_thread_main(void* user_data)
{
    EngineAppSimThreadContext* context = (EngineAppSimThreadContext*)user_data;
    EngineAppContext* app;
    EngineInput sim_input;
    EngineInputBindings bindings;
    EngineInputSnapshot empty_snapshot;
    EngineRuntimeInputPacket latest_input;
    SceneRenderSnapshotBuilder* snapshot_builder;
    uint64_t last_tick_ms;
    uint64_t last_snapshot_publish_ms;
    uint64_t last_input_frame_id = 0U;
    bool has_input = false;

    if (context == NULL || context->app == NULL || context->render_snapshot_exchange == NULL || context->input_stream == NULL)
    {
        return NULL;
    }

    app = context->app;
    EngineInputBindings_set_defaults(&bindings);
    EngineInput_init(&sim_input, &bindings);
    memset(&empty_snapshot, 0, sizeof(empty_snapshot));
    memset(&latest_input, 0, sizeof(latest_input));
    snapshot_builder = scene_render_snapshot_builder_create();
    if (snapshot_builder == NULL)
    {
        return NULL;
    }
    engine_app_sim_get_scene_step_config(context);
    last_tick_ms = (uint64_t)engine_app_sim_now_ms();
    last_snapshot_publish_ms = last_tick_ms;

    while (!atomic_load_explicit(&context->shutdown_requested, memory_order_acquire))
    {
        EngineRuntimeInputPacket packet;
        RenderSnapshotBuffer* next_render_snapshot;
        uint64_t now_ms;
        double dt_seconds;
        double update_started_ms;
        double update_ms;
        double fixed_step_started_ms;
        double fixed_step_wall_ms;
        double input_started_ms;
        double input_ms;
        double game_update_started_ms;
        double game_update_ms;
        double drag_started_ms;
        double drag_ms;
        double snapshot_acquire_started_ms;
        double snapshot_acquire_ms;
        double snapshot_build_started_ms;
        double snapshot_build_ms;
        bool input_changed = false;
        bool input_captured = false;
        bool snapshot_publish_due;
        uint32_t physics_substeps;
        float render_alpha;
        Aabb snapshot_view_bounds;

        if (input_packet_stream_read_latest(context->input_stream, &packet))
        {
            latest_input = packet;
            has_input = true;
            input_changed = packet.frame_id != last_input_frame_id;
            last_input_frame_id = packet.frame_id;
            context->cached_debug_overlay_enabled = latest_input.debug_overlay_enabled;
            context->cached_draw_debug_world = latest_input.draw_debug_world;
        }

        now_ms = (uint64_t)engine_app_sim_now_ms();
        dt_seconds = (double)(now_ms - last_tick_ms) / 1000.0;
        if (dt_seconds < 0.0) dt_seconds = 0.0;
        if (dt_seconds > 0.1) dt_seconds = 0.1;
        last_tick_ms = now_ms;
        update_started_ms = engine_app_sim_now_ms();

        input_started_ms = engine_app_sim_now_ms();
        if (input_changed || context->accumulator_seconds + dt_seconds >= context->fixed_dt_seconds)
        {
            EngineInput_capture(&sim_input, has_input ? &latest_input.snapshot : &empty_snapshot);
            input_captured = true;
        }
        input_ms = engine_app_sim_now_ms() - input_started_ms;

        game_update_started_ms = engine_app_sim_now_ms();
        if (context->desc->update_sim != NULL)
        {
            context->desc->update_sim(app, dt_seconds, &sim_input, context->desc->user_data);
        }
        game_update_ms = engine_app_sim_now_ms() - game_update_started_ms;

        fixed_step_started_ms = engine_app_sim_now_ms();
        physics_substeps = engine_app_sim_step_scene(context, dt_seconds, &sim_input);
        fixed_step_wall_ms = engine_app_sim_now_ms() - fixed_step_started_ms;

        drag_started_ms = engine_app_sim_now_ms();
        if (has_input && (latest_input.drag_entity_active || latest_input.drag_entity_release))
        {
            InteractionSystem_ApplyDragPacket(app->scene, &latest_input);
        }
        drag_ms = engine_app_sim_now_ms() - drag_started_ms;

        snapshot_publish_due =
            input_changed ||
            latest_input.drag_entity_active ||
            latest_input.drag_entity_release ||
            now_ms - last_snapshot_publish_ms >= context->settings->app_snapshot_publish_interval_ms;
        if (!snapshot_publish_due)
        {
            if (physics_substeps == 0U)
            {
                engine_app_sim_sleep_ms(context->settings->app_idle_sleep_ms);
            }
            continue;
        }

        render_alpha = engine_app_sim_compute_render_alpha(context);
        snapshot_acquire_started_ms = engine_app_sim_now_ms();
        next_render_snapshot = render_snapshot_exchange_begin_write(context->render_snapshot_exchange);
        snapshot_acquire_ms = engine_app_sim_now_ms() - snapshot_acquire_started_ms;
        if (next_render_snapshot == NULL)
        {
            engine_app_sim_sleep_ms(context->settings->app_idle_sleep_ms);
            continue;
        }

        snapshot_view_bounds = scene_render_snapshot_compute_view_bounds(
            app->scene,
            has_input && latest_input.camera_view_width > 0.0f
                ? &(Camera){
                    .x = latest_input.camera_x,
                    .y = latest_input.camera_y,
                    .view_width = latest_input.camera_view_width,
                    .view_height = latest_input.camera_view_height
                }
                : renderer_get_camera_const(app->renderer),
            context->settings->app_snapshot_preload_screens
        );

        snapshot_build_started_ms = engine_app_sim_now_ms();
        if (!scene_render_snapshot_build(
            snapshot_builder,
            app->scene,
            snapshot_view_bounds,
            render_alpha,
            now_ms,
            0.0,
            physics_substeps,
            context->cached_debug_overlay_enabled,
            context->cached_draw_debug_world,
            latest_input.selected_entity_id,
            latest_input.hovered_entity_id,
            context->desc->backdrop_draw,
            context->desc->backdrop_user_data,
            context->desc->backdrop_signature != NULL
                ? context->desc->backdrop_signature(context->desc->backdrop_user_data)
                : 0U,
            next_render_snapshot
        ))
        {
            engine_app_sim_sleep_ms(context->settings->app_idle_sleep_ms);
            continue;
        }
        snapshot_build_ms = engine_app_sim_now_ms() - snapshot_build_started_ms;
        update_ms = engine_app_sim_now_ms() - update_started_ms;
        next_render_snapshot->stats.overlay.update_ms = (float)fmax(update_ms - fixed_step_wall_ms, 0.0);
        next_render_snapshot->stats.overlay.sim_ms = (float)update_ms;
        next_render_snapshot->stats.sim.input_ms = input_captured ? (float)input_ms : 0.0f;
        next_render_snapshot->stats.sim.game_update_ms = (float)game_update_ms;
        next_render_snapshot->stats.sim.fixed_step_wall_ms = (float)fixed_step_wall_ms;
        next_render_snapshot->stats.sim.drag_ms = (float)drag_ms;
        next_render_snapshot->stats.sim.snapshot_acquire_ms = (float)snapshot_acquire_ms;
        next_render_snapshot->stats.sim.snapshot_build_ms = (float)snapshot_build_ms;
        render_snapshot_exchange_publish(context->render_snapshot_exchange, next_render_snapshot);
        last_snapshot_publish_ms = now_ms;
    }

    scene_render_snapshot_builder_destroy(snapshot_builder);
    return NULL;
}
