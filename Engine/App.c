#include "App.h"

#include "AppInternal.h"
#include "Core/InputPacketStream.h"
#include "Runtime/AppRenderLoop.h"
#include "Runtime/AppSimulation.h"
#include "RuntimeInput.h"
#include "RuntimeConfig.h"
#include "Settings.h"
#include "Rendering/RaylibBackend.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderSnapshot.h"
#include "Rendering/ResourceCommandQueue.h"
#include "Scene/Scene.h"
#include "Core/TaskSystem.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void EngineAppDesc_InitDefaults(EngineAppDesc* desc)
{
    if (desc == NULL)
    {
        return;
    }

    memset(desc, 0, sizeof(*desc));
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
    runtime_config_from_engine_settings(&runtime_config, settings);
    InteractionSystem_Init(&app.interaction_state);

    if (!input_packet_stream_init(&input_stream, sizeof(EngineRuntimeInputPacket)))
    {
        goto cleanup;
    }
    if (!render_snapshot_exchange_init(&render_snapshot_exchange))
    {
        goto cleanup;
    }
    app.resource_command_queue = resource_command_queue_create(0U);
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
        "runtime initialized cores=%d physics=%d prebake_target_fps=%.1f",
        task_system_get_online_core_count(&app.task_system),
        task_system_get_worker_count(&app.task_system),
        runtime_config.renderer.prebake_target_fps
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
    sim_context.visible_query_entity_capacity = 0U;
    sim_context.visible_query_entities = NULL;
    atomic_init(&sim_context.shutdown_requested, false);
    if (pthread_create(&sim_thread, NULL, engine_app_simulation_thread_main, &sim_context) != 0)
    {
        goto cleanup;
    }
    sim_thread_started = true;

    engine_app_run_render_loop(&app, settings, &render_snapshot_exchange, &input_stream);
    exit_code = 0;

cleanup:
    if (sim_thread_started)
    {
        atomic_store_explicit(&sim_context.shutdown_requested, true, memory_order_release);
        pthread_join(sim_thread, NULL);
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
