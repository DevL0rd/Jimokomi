#include "App.h"

#include "Core/InputPacketStream.h"
#include "RuntimeConfig.h"
#include "Settings.h"
#include "Rendering/DebugOverlay.h"
#include "Rendering/RaylibBackend.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderSnapshot.h"
#include "Rendering/ResourceCommandQueue.h"
#include "Rendering/ResourceManager.h"
#include "Rendering/SceneRenderSnapshot.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Scene/Systems/InteractionSystem.h"
#include "Core/TaskSystem.h"

#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct EngineAppContext
{
    Engine engine;
    RaylibBackend backend;
    Renderer* renderer;
    Scene* scene;
    TaskSystem task_system;
    ResourceCommandQueue* resource_command_queue;
    InteractionSystemState interaction_state;
    uint64_t dropped_render_snapshots;
};

typedef struct EngineAppSimThreadContext
{
    EngineAppContext* app;
    const EngineAppDesc* desc;
    const EngineSettings* settings;
    RenderSnapshotExchange* render_snapshot_exchange;
    InputPacketStream* input_stream;
    atomic_bool shutdown_requested;
    Entity** visible_query_entities;
    size_t visible_query_entity_capacity;
    double accumulator_seconds;
    double fixed_dt_seconds;
    uint32_t max_substeps;
    bool cached_debug_overlay_enabled;
    bool cached_draw_debug_world;
} EngineAppSimThreadContext;

static double engine_app_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

Engine* EngineApp_GetEngine(EngineAppContext* app)
{
    return app != NULL ? &app->engine : NULL;
}

Renderer* EngineApp_GetRenderer(EngineAppContext* app)
{
    return app != NULL ? app->renderer : NULL;
}

Scene* EngineApp_GetScene(EngineAppContext* app)
{
    return app != NULL ? app->scene : NULL;
}

TaskSystem* EngineApp_GetTaskSystem(EngineAppContext* app)
{
    return app != NULL ? &app->task_system : NULL;
}

static void engine_app_sleep_ms(uint32_t milliseconds)
{
    struct timespec request;

    request.tv_sec = (time_t)(milliseconds / 1000U);
    request.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    nanosleep(&request, NULL);
}

void EngineAppDesc_InitDefaults(EngineAppDesc* desc)
{
    if (desc == NULL)
    {
        return;
    }

    memset(desc, 0, sizeof(*desc));
}

static void engine_app_get_scene_step_config(EngineAppSimThreadContext* context)
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

static float engine_app_compute_render_alpha(const EngineAppSimThreadContext* context)
{
    if (context == NULL || context->fixed_dt_seconds <= 0.0)
    {
        return 1.0f;
    }

    return clamp_f((float)(context->accumulator_seconds / context->fixed_dt_seconds), 0.0f, 1.0f);
}

static uint32_t engine_app_step_scene(
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

static void* engine_app_simulation_thread_main(void* user_data)
{
    EngineAppSimThreadContext* context = (EngineAppSimThreadContext*)user_data;
    EngineAppContext* app;
    EngineInput sim_input;
    EngineInputBindings bindings;
    EngineInputSnapshot empty_snapshot;
    EngineRuntimeInputPacket latest_input;
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
    engine_app_get_scene_step_config(context);
    last_tick_ms = (uint64_t)engine_app_now_ms();
    last_snapshot_publish_ms = last_tick_ms;

    while (!atomic_load_explicit(&context->shutdown_requested, memory_order_acquire))
    {
        EngineRuntimeInputPacket packet;
        RenderSnapshotBuffer* next_render_snapshot;
        PhysicsWorldSnapshot physics_snapshot;
        SceneRenderSnapshotDesc snapshot_desc;
        SceneRenderSnapshotProfile cull_profile;
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

        now_ms = (uint64_t)engine_app_now_ms();
        dt_seconds = (double)(now_ms - last_tick_ms) / 1000.0;
        if (dt_seconds < 0.0) dt_seconds = 0.0;
        if (dt_seconds > 0.1) dt_seconds = 0.1;
        last_tick_ms = now_ms;
        update_started_ms = engine_app_now_ms();

        input_started_ms = engine_app_now_ms();
        if (input_changed || context->accumulator_seconds + dt_seconds >= context->fixed_dt_seconds)
        {
            EngineInput_capture(&sim_input, has_input ? &latest_input.snapshot : &empty_snapshot);
            input_captured = true;
        }
        input_ms = engine_app_now_ms() - input_started_ms;

        game_update_started_ms = engine_app_now_ms();
        if (context->desc->update_sim != NULL)
        {
            context->desc->update_sim(app, dt_seconds, &sim_input, context->desc->user_data);
        }
        game_update_ms = engine_app_now_ms() - game_update_started_ms;

        fixed_step_started_ms = engine_app_now_ms();
        physics_substeps = engine_app_step_scene(context, dt_seconds, &sim_input);
        fixed_step_wall_ms = engine_app_now_ms() - fixed_step_started_ms;

        drag_started_ms = engine_app_now_ms();
        if (has_input && (latest_input.drag_entity_active || latest_input.drag_entity_release))
        {
            InteractionSystem_ApplyDragPacket(app->scene, &latest_input);
        }
        drag_ms = engine_app_now_ms() - drag_started_ms;

        snapshot_publish_due =
            input_changed ||
            latest_input.drag_entity_active ||
            latest_input.drag_entity_release ||
            now_ms - last_snapshot_publish_ms >= context->settings->app_snapshot_publish_interval_ms;
        if (!snapshot_publish_due)
        {
            if (physics_substeps == 0U)
            {
                engine_app_sleep_ms(context->settings->app_idle_sleep_ms);
            }
            continue;
        }

        render_alpha = engine_app_compute_render_alpha(context);
        snapshot_acquire_started_ms = engine_app_now_ms();
        next_render_snapshot = render_snapshot_exchange_begin_write(context->render_snapshot_exchange);
        snapshot_acquire_ms = engine_app_now_ms() - snapshot_acquire_started_ms;
        if (next_render_snapshot == NULL)
        {
            engine_app_sleep_ms(context->settings->app_idle_sleep_ms);
            continue;
        }

        memset(&physics_snapshot, 0, sizeof(physics_snapshot));
        if (app->scene != NULL && Scene_GetPhysicsWorld(app->scene) != NULL)
        {
            PhysicsWorld_GetSnapshot(Scene_GetPhysicsWorld(app->scene), &physics_snapshot);
            if (physics_substeps == 0U)
            {
                physics_snapshot.profile_step_ms = 0.0f;
                physics_snapshot.profile_pairs_ms = 0.0f;
                physics_snapshot.profile_collide_ms = 0.0f;
                physics_snapshot.profile_solve_ms = 0.0f;
            }
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

        memset(&snapshot_desc, 0, sizeof(snapshot_desc));
        snapshot_desc.scene = app->scene;
        snapshot_desc.scratch_entities = context->visible_query_entities;
        snapshot_desc.scratch_entity_capacity = context->visible_query_entity_capacity;
        snapshot_desc.view_bounds = snapshot_view_bounds;
        snapshot_desc.render_alpha = render_alpha;
        snapshot_desc.now_ms = now_ms;
        snapshot_desc.update_ms = 0.0;
        snapshot_desc.physics_substeps = physics_substeps;
        snapshot_desc.debug_overlay_enabled = context->cached_debug_overlay_enabled;
        snapshot_desc.draw_debug_world = context->cached_draw_debug_world;
        snapshot_desc.selected_entity_id = latest_input.selected_entity_id;
        snapshot_desc.hovered_entity_id = latest_input.hovered_entity_id;
        snapshot_desc.backdrop_draw = context->desc->backdrop_draw;
        snapshot_desc.backdrop_user_data = context->desc->backdrop_user_data;
        snapshot_desc.backdrop_signature = context->desc->backdrop_signature != NULL
            ? context->desc->backdrop_signature(context->desc->backdrop_user_data)
            : 0U;
        snapshot_desc.physics_snapshot = &physics_snapshot;
        snapshot_desc.profile_culling = app->engine.profiler.enabled;

        snapshot_build_started_ms = engine_app_now_ms();
        scene_render_snapshot_build(&snapshot_desc, next_render_snapshot, &cull_profile);
        snapshot_build_ms = engine_app_now_ms() - snapshot_build_started_ms;
        update_ms = engine_app_now_ms() - update_started_ms;
        next_render_snapshot->stats.overlay.update_ms = (float)fmax(update_ms - fixed_step_wall_ms, 0.0);
        next_render_snapshot->stats.overlay.sim_ms = (float)update_ms;
        next_render_snapshot->stats.sim_input_ms = input_captured ? (float)input_ms : 0.0f;
        next_render_snapshot->stats.sim_spawn_ms = (float)game_update_ms;
        next_render_snapshot->stats.sim_fixed_step_wall_ms = (float)fixed_step_wall_ms;
        next_render_snapshot->stats.sim_drag_ms = (float)drag_ms;
        next_render_snapshot->stats.sim_snapshot_acquire_ms = (float)snapshot_acquire_ms;
        next_render_snapshot->stats.sim_snapshot_build_ms = (float)snapshot_build_ms;
        if (app->resource_command_queue != NULL)
        {
            next_render_snapshot->stats.resource_commands_enqueued =
                resource_command_queue_get_enqueued_count(app->resource_command_queue);
            next_render_snapshot->stats.resource_commands_dropped =
                resource_command_queue_get_dropped_count(app->resource_command_queue);
        }
        render_snapshot_exchange_publish(context->render_snapshot_exchange, next_render_snapshot);
        last_snapshot_publish_ms = now_ms;
    }

    return NULL;
}

static void engine_app_emit_profiler_metadata(
    EngineAppContext* app,
    const RenderSnapshotBuffer* render_snapshot,
    const RendererFrame* frame,
    size_t drained_resource_commands
)
{
    RendererStatsSnapshot renderer_stats;
    DebugOverlayStatsSnapshot debug_stats;
    SceneStatsSnapshot scene_stats;
    SpatialGridStatsSnapshot spatial_stats;
    ResourceManagerStatsSnapshot resource_stats;

    if (app == NULL || render_snapshot == NULL || frame == NULL)
    {
        return;
    }

    renderer_get_stats_snapshot(app->renderer, &renderer_stats);
    debug_overlay_get_stats_snapshot(renderer_get_debug_overlay(app->renderer), &debug_stats);
    Scene_GetStatsSnapshot(app->scene, &scene_stats);
    Scene_GetSpatialGridStatsSnapshot(app->scene, &spatial_stats);
    resource_manager_get_stats_snapshot(renderer_get_resource_manager(app->renderer), &resource_stats);

    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_items", (double)frame->item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "visible_candidates", (double)render_snapshot->world.item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "cull_grid_ms", render_snapshot->stats.cull_grid_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "cull_grid_candidates", (double)render_snapshot->stats.cull_grid_candidates);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_entries", (double)spatial_stats.entry_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_nodes", (double)spatial_stats.node_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_rebuild_ms", spatial_stats.rebuild_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_incremental_ms", spatial_stats.incremental_update_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_query_ms", spatial_stats.query_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_query_cells", (double)spatial_stats.query_cells);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_query_hits", (double)spatial_stats.query_hits);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "spatial_grid_dirty_cells", (double)spatial_stats.dirty_cell_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_substeps", (double)render_snapshot->stats.physics_substeps);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_ms", render_snapshot->stats.overlay.physics_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_ms", render_snapshot->stats.overlay.sim_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_total_bodies", (double)render_snapshot->stats.overlay.total_body_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_sleeping_bodies", (double)render_snapshot->stats.overlay.sleeping_body_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_moved_bodies", (double)render_snapshot->stats.overlay.moved_body_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_input_ms", render_snapshot->stats.sim_input_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_game_update_ms", render_snapshot->stats.sim_spawn_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_fixed_step_wall_ms", render_snapshot->stats.sim_fixed_step_wall_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_drag_ms", render_snapshot->stats.sim_drag_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_snapshot_acquire_ms", render_snapshot->stats.sim_snapshot_acquire_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "sim_snapshot_build_ms", render_snapshot->stats.sim_snapshot_build_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_input_route_ms", scene_stats.input_route_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_random_force_ms", scene_stats.random_force_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_physics_sync_ms", scene_stats.physics_sync_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_spatial_grid_ms", scene_stats.spatial_grid_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "scene_camera_follow_ms", scene_stats.camera_follow_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_alpha", render_snapshot->stats.render_alpha);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "physics_hz", render_snapshot->stats.physics_hz);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_snapshot_sequence", (double)render_snapshot->sequence);
    EngineProfiler_set_metadata_number(
        &app->engine.profiler,
        "render_snapshot_age_ms",
        render_snapshot->published_at_ms > 0U && frame->now_ms >= render_snapshot->published_at_ms
            ? (double)(frame->now_ms - render_snapshot->published_at_ms)
            : 0.0
    );
    EngineProfiler_set_metadata_number(&app->engine.profiler, "render_snapshot_dropped", (double)app->dropped_render_snapshots);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_x", render_snapshot->stats.camera_x);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_y", render_snapshot->stats.camera_y);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_visual_sources", (double)resource_stats.visual_source_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_materials", (double)resource_stats.material_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_shaders", (double)resource_stats.shader_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_baked_surfaces", (double)resource_stats.baked_surface_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_pending_bakes", (double)resource_stats.pending_bake_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_drained", (double)drained_resource_commands);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_enqueued_total", (double)render_snapshot->stats.resource_commands_enqueued);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_dropped_total", (double)render_snapshot->stats.resource_commands_dropped);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "resource_command_deduped_total", (double)resource_command_queue_get_deduped_count(app->resource_command_queue));
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_last_items", (double)renderer_stats.render_item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_last_sprites", (double)renderer_stats.sprite_draw_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_visible_items", (double)renderer_stats.visible_item_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_instanced_batches", (double)renderer_stats.instanced_batch_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_instanced_draws", (double)renderer_stats.instanced_draw_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_sort_ms", renderer_stats.sort_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_visibility_ms", renderer_stats.visibility_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_body_ms", renderer_stats.body_draw_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_overlay_ms", renderer_stats.overlay_draw_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "renderer_instance_ms", renderer_stats.instance_draw_ms);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "debug_visible_entities", (double)debug_stats.visible_entity_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "debug_active_collisions", (double)debug_stats.active_collision_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "hover_pick_queries", (double)app->interaction_state.hover_pick_query_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "hover_pick_skips", (double)app->interaction_state.hover_pick_skip_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "hover_query_dirty", (double)app->interaction_state.hover_query_dirty_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_rect_dirty", (double)app->interaction_state.camera_rect_dirty_count);
    EngineProfiler_set_metadata_number(&app->engine.profiler, "camera_rect_stable", (double)app->interaction_state.camera_rect_stable_count);
    EngineProfiler_set_metadata_bool(&app->engine.profiler, "debug_enabled", debug_stats.draw_ui);
}

int EngineApp_Run(const EngineAppDesc* desc)
{
    EngineAppContext app;
    EngineAppSimThreadContext sim_context;
    InputPacketStream input_stream;
    RenderSnapshotExchange render_snapshot_exchange;
    RuntimeConfig runtime_config;
    const EngineSettings* settings = EngineSettings_GetDefaults();
    RendererConfig renderer_config;
    TaskSystemConfig task_system_config;
    pthread_t sim_thread;
    bool sim_thread_started = false;
    uint64_t last_render_snapshot_sequence = 0U;
    const RenderSnapshotBuffer* retained_render_snapshot = NULL;
    char log_message[128];
    int exit_code = 1;

    if (desc == NULL || desc->create_scene == NULL)
    {
        return 1;
    }

    memset(&app, 0, sizeof(app));
    memset(&sim_context, 0, sizeof(sim_context));
    memset(&input_stream, 0, sizeof(input_stream));
    memset(&render_snapshot_exchange, 0, sizeof(render_snapshot_exchange));
    runtime_config_init_defaults(&runtime_config);
    InteractionSystem_Init(&app.interaction_state);

    if (!input_packet_stream_init(&input_stream, sizeof(EngineRuntimeInputPacket)))
    {
        goto cleanup;
    }
    if (!render_snapshot_exchange_init(
            &render_snapshot_exchange,
            settings->app_render_item_capacity,
            settings->app_debug_entity_capacity,
            settings->app_debug_collision_capacity))
    {
        goto cleanup;
    }
    app.resource_command_queue = resource_command_queue_create(settings->app_render_item_capacity * 4U);
    if (app.resource_command_queue == NULL)
    {
        goto cleanup;
    }
    if (!raylib_backend_init(&app.backend, runtime_config.window_width, runtime_config.window_height, runtime_config.window_title))
    {
        goto cleanup;
    }
    if (!Engine_init(&app.engine, &runtime_config.engine))
    {
        goto cleanup;
    }
    memset(&task_system_config, 0, sizeof(task_system_config));
    task_system_config.requested_worker_count = 0;
    if (!task_system_init(&app.task_system, &task_system_config))
    {
        goto cleanup;
    }
    snprintf(
        log_message,
        sizeof(log_message),
        "runtime initialized cores=%d physics=%d prebake_budget=%zu",
        task_system_get_online_core_count(&app.task_system),
        task_system_get_worker_count(&app.task_system),
        runtime_config.renderer.prebake_budget_per_frame
    );
    EngineLogger_info(&app.engine.logger, log_message, "runtime");

    renderer_config = runtime_config.renderer;
    app.renderer = renderer_create(&app.backend.render_backend, &renderer_config);
    if (app.renderer == NULL)
    {
        goto cleanup;
    }
    if (desc->register_resources != NULL && !desc->register_resources(&app, desc->user_data))
    {
        goto cleanup;
    }
    app.scene = desc->create_scene(&app, desc->user_data);
    if (app.scene == NULL)
    {
        goto cleanup;
    }
    {
        SceneView scene_view = Scene_GetViewSnapshot(app.scene);
        Camera* render_camera = renderer_get_camera(app.renderer);
        if (render_camera != NULL)
        {
            int viewport_width = 0;
            int viewport_height = 0;
            renderer_get_viewport_size(app.renderer, &viewport_width, &viewport_height);
            render_camera->x = scene_view.x;
            render_camera->y = scene_view.y;
            render_camera->view_width = scene_view.view_width;
            render_camera->view_height = scene_view.view_height;
            render_camera->viewport_width = (float)viewport_width;
            render_camera->viewport_height = (float)viewport_height;
            if (scene_view.has_world_bounds)
            {
                camera_set_world_bounds(
                    render_camera,
                    (Rect){
                        scene_view.world_min_x,
                        scene_view.world_min_y,
                        scene_view.world_max_x - scene_view.world_min_x,
                        scene_view.world_max_y - scene_view.world_min_y
                    }
                );
            }
        }
    }

    sim_context.app = &app;
    sim_context.desc = desc;
    sim_context.settings = settings;
    sim_context.render_snapshot_exchange = &render_snapshot_exchange;
    sim_context.input_stream = &input_stream;
    sim_context.visible_query_entity_capacity = settings->app_render_item_capacity;
    sim_context.visible_query_entities = (Entity**)calloc(sim_context.visible_query_entity_capacity, sizeof(*sim_context.visible_query_entities));
    if (sim_context.visible_query_entities == NULL)
    {
        goto cleanup;
    }
    atomic_init(&sim_context.shutdown_requested, false);
    if (pthread_create(&sim_thread, NULL, engine_app_simulation_thread_main, &sim_context) != 0)
    {
        goto cleanup;
    }
    sim_thread_started = true;

    while (!raylib_backend_should_close(&app.backend))
    {
        const RenderSnapshotBuffer* render_snapshot;
        EngineRuntimeInputPacket* next_input_packet;
        DebugOverlaySnapshot debug_overlay_snapshot;
        EngineStatsSnapshot engine_stats_snapshot;
        size_t drained_resource_commands;
        EngineInputSnapshot input_snapshot;
        RendererFrame frame;
        const DebugEntityView* selected_entity;
        bool has_selection;
        bool pointer_over_ui;

        raylib_backend_pump_events(&app.backend);
        input_snapshot = raylib_backend_snapshot_input(&app.backend);
        Engine_update(&app.engine, 0.0, &input_snapshot);
        if (EngineInput_was_pressed(&app.engine.input, ENGINE_INPUT_ACTION_DEBUG_TOGGLE))
        {
            debug_overlay_toggle(renderer_get_debug_overlay(app.renderer));
        }
        if (EngineInput_was_pressed(&app.engine.input, ENGINE_INPUT_ACTION_DEBUG_WORLD_TOGGLE))
        {
            debug_overlay_toggle_world_gizmos(renderer_get_debug_overlay(app.renderer));
        }

        engine_stats_snapshot = Engine_get_stats_snapshot(&app.engine);
        {
            const RenderSnapshotBuffer* next_render_snapshot =
                render_snapshot_exchange_acquire_if_new(&render_snapshot_exchange, last_render_snapshot_sequence);
            if (next_render_snapshot != NULL)
            {
                if (retained_render_snapshot != NULL)
                {
                    render_snapshot_exchange_release_published(&render_snapshot_exchange, retained_render_snapshot);
                }
                if (last_render_snapshot_sequence > 0U &&
                    next_render_snapshot->sequence > last_render_snapshot_sequence + 1U)
                {
                    app.dropped_render_snapshots += next_render_snapshot->sequence - last_render_snapshot_sequence - 1U;
                }
                last_render_snapshot_sequence = next_render_snapshot->sequence;
                retained_render_snapshot = next_render_snapshot;
            }
        }
        render_snapshot = retained_render_snapshot;
        if (render_snapshot == NULL)
        {
            engine_app_sleep_ms(settings->app_idle_sleep_ms);
            continue;
        }

        selected_entity = render_snapshot->world.has_selected_entity ? &render_snapshot->world.selected_entity : NULL;
        has_selection = selected_entity != NULL || app.interaction_state.selected_entity_id != 0U;
        debug_overlay_handle_input(
            renderer_get_debug_overlay(app.renderer),
            &app.engine.input,
            has_selection,
            app.backend.window_width,
            app.backend.window_height
        );
        pointer_over_ui = debug_overlay_is_pointer_over_ui(
            renderer_get_debug_overlay(app.renderer),
            has_selection,
            (float)input_snapshot.mouse_x,
            (float)input_snapshot.mouse_y,
            app.backend.window_width,
            app.backend.window_height
        );
        InteractionSystem_UpdateRender(
            &app.interaction_state,
            NULL,
            app.renderer,
            &app.engine.input,
            render_snapshot,
            pointer_over_ui,
            engine_stats_snapshot.frame_ms > 0.0 ? engine_stats_snapshot.frame_ms / 1000.0 : 0.0
        );

        next_input_packet = input_packet_stream_begin_write(&input_stream);
        if (next_input_packet != NULL)
        {
            InteractionSystem_WriteInputPacket(
                &app.interaction_state,
                &input_snapshot,
                app.renderer,
                pointer_over_ui,
                debug_overlay_is_ui_visible(renderer_get_debug_overlay(app.renderer)),
                debug_overlay_is_world_gizmos_visible(renderer_get_debug_overlay(app.renderer)),
                app.backend.window_width,
                app.backend.window_height,
                input_packet_stream_next_frame_id(&input_stream),
                next_input_packet
            );
            input_packet_stream_publish(&input_stream, next_input_packet, next_input_packet->frame_id);
        }
        InteractionSystem_ClearReleasedDrag(&app.interaction_state, &app.engine.input);

        drained_resource_commands = renderer_drain_resource_commands(app.renderer, app.resource_command_queue);
        render_world_snapshot_build_frame(&render_snapshot->world, &frame);
        selected_entity = render_snapshot->world.has_selected_entity ? &render_snapshot->world.selected_entity : NULL;
        debug_overlay_snapshot = render_snapshot->stats.overlay;
        debug_overlay_snapshot.fps = (float)engine_stats_snapshot.fps;
        debug_overlay_snapshot.draw_ms = (float)engine_stats_snapshot.draw_ms;
        debug_overlay_snapshot.snapshot_age_ms =
            render_snapshot->published_at_ms > 0U && frame.now_ms >= render_snapshot->published_at_ms
                ? (float)(frame.now_ms - render_snapshot->published_at_ms)
                : 0.0f;

        Engine_draw_begin(&app.engine);
        raylib_backend_begin_frame(&app.backend, settings->app_clear_color);
        frame.selected_debug_entity_id = selected_entity != NULL ? selected_entity->id : app.interaction_state.selected_entity_id;
        frame.hovered_debug_entity_id = app.interaction_state.hovered_entity_id;
        EngineProfiler_begin_scope(&app.engine.profiler, "draw.renderer");
        renderer_draw(app.renderer, &app.backend.render_backend, &frame);
        EngineProfiler_end_scope(&app.engine.profiler, "draw.renderer");
        EngineProfiler_begin_scope(&app.engine.profiler, "draw.debug_ui");
        debug_overlay_draw_ui(
            renderer_get_debug_overlay(app.renderer),
            &app.backend.render_backend,
            &debug_overlay_snapshot,
            &engine_stats_snapshot,
            selected_entity,
            app.backend.window_width,
            app.backend.window_height
        );
        EngineProfiler_end_scope(&app.engine.profiler, "draw.debug_ui");
        engine_app_emit_profiler_metadata(&app, render_snapshot, &frame, drained_resource_commands);
        raylib_backend_end_frame(&app.backend);
        Engine_draw_end(&app.engine);
    }

    exit_code = 0;

cleanup:
    if (sim_thread_started)
    {
        atomic_store_explicit(&sim_context.shutdown_requested, true, memory_order_release);
        pthread_join(sim_thread, NULL);
    }
    if (retained_render_snapshot != NULL)
    {
        render_snapshot_exchange_release_published(&render_snapshot_exchange, retained_render_snapshot);
    }
    if (app.scene != NULL)
    {
        Scene_Destroy(app.scene);
    }
    if (app.renderer != NULL)
    {
        renderer_destroy(app.renderer);
    }
    task_system_dispose(&app.task_system);
    Engine_dispose(&app.engine);
    raylib_backend_dispose(&app.backend);
    resource_command_queue_destroy(app.resource_command_queue);
    render_snapshot_exchange_dispose(&render_snapshot_exchange);
    input_packet_stream_dispose(&input_stream);
    free(sim_context.visible_query_entities);
    return exit_code;
}
