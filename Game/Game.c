#include "Game/Game.h"
#include "Game/BallVisuals.h"

#include "../Engine/Engine.h"
#include "../Engine/Core/RuntimeConfig.h"
#include "../Engine/Core/TaskSystem.h"
#include "../Engine/Scene/Scene.h"
#include "../Engine/Scene/Entity.h"
#include "../Engine/Scene/Components/CameraTargetComponent.h"
#include "../Engine/Scene/Components/ColliderComponent.h"
#include "../Engine/Scene/Components/RigidBodyComponent.h"
#include "../Engine/Scene/Components/TransformComponent.h"
#include "../Engine/Rendering/RaylibBackend.h"
#include "../Engine/Rendering/Renderer.h"
#include "../third_party/box2d/include/box2d/box2d.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BALL_COUNT 100
#define SOURCE_VARIANT_COUNT 8
#define INVALID_INDEX ((size_t)-1)
#define PLAYER_INDEX 0U
#define WORLD_WIDTH 1920.0f
#define WORLD_HEIGHT 1080.0f
#define GRID_CELL_SIZE 96.0f
#define PLAYER_MOVE_SPEED 160.0f
#define PLAYER_MOVE_SMOOTHING 0.085f
#define PLAYER_JUMP_IMPULSE 585.0f
#define WORLD_WALL_THICKNESS 32.0f

typedef struct Ball {
    Entity* entity;
    float radius;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
} Ball;

typedef struct GameState {
    Engine engine;
    RaylibBackend backend;
    Renderer renderer;
    Scene* scene;
    TaskSystem task_system;
    Ball balls[BALL_COUNT];
    uint32_t visible_indices[BALL_COUNT];
    size_t tracked_ball_index;
    size_t selected_ball_index;
    size_t dragged_ball_index;
    Vec2 drag_last_world;
    Vec2 drag_release_velocity;
    bool drag_active;
    double accumulator_seconds;
    uint64_t last_tick_ms;
    WorldBackdropConfig backdrop;
    ResourceHandle shared_ball_shader_handle;
    ResourceHandle ball_source_handles[SOURCE_VARIANT_COUNT];
    ResourceHandle ball_material_handles[BALL_COUNT];
} GameState;

static bool game_player_is_grounded(const GameState* game);

static TransformComponent* game_get_transform(Entity* entity) {
    return entity != NULL ? (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM) : NULL;
}

static RigidBodyComponent* game_get_rigid_body(Entity* entity) {
    return entity != NULL ? (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY) : NULL;
}

static ColliderComponent* game_get_collider(Entity* entity) {
    return entity != NULL ? (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER) : NULL;
}

static size_t game_find_ball_index(const GameState* game, const Entity* entity) {
    size_t index;

    if (game == NULL || entity == NULL) {
        return INVALID_INDEX;
    }

    for (index = 0U; index < BALL_COUNT; ++index) {
        if (game->balls[index].entity == entity) {
            return index;
        }
    }

    return INVALID_INDEX;
}

static double game_now_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static bool game_drain_required_prebakes(GameState* game) {
    size_t total_required;
    size_t previous_budget;

    if (game == NULL) {
        return false;
    }

    total_required = resource_manager_get_pending_bake_count(&game->renderer.resource_manager);
    if (total_required == 0U) {
        return true;
    }
    previous_budget = game->renderer.resource_manager.bake_budget_per_frame;
    game->renderer.resource_manager.bake_budget_per_frame = 32U;

    while (resource_manager_get_pending_bake_count(&game->renderer.resource_manager) > 0U) {
        size_t pending_count = resource_manager_get_pending_bake_count(&game->renderer.resource_manager);
        size_t completed_count = total_required > pending_count ? total_required - pending_count : 0U;
        float progress = total_required > 0U ? (float)completed_count / (float)total_required : 1.0f;
        Target target;
        char label[96];

        raylib_backend_pump_events(&game->backend);
        if (raylib_backend_should_close(&game->backend)) {
            game->renderer.resource_manager.bake_budget_per_frame = previous_budget;
            return false;
        }

        raylib_backend_begin_frame(&game->backend, (Color32){ 0x0b1020U });
        target_init(&target, &game->backend.render_backend, 0.0f, 0.0f);
        target_rect_filled(&target, (Rect){ 180.0f, 230.0f, 600.0f, 84.0f }, (Color32){ 0x101828U });
        target_rect_filled(&target, (Rect){ 196.0f, 276.0f, 568.0f * progress, 16.0f }, (Color32){ 0x60a5faU });
        target_rect(&target, (Rect){ 196.0f, 276.0f, 568.0f, 16.0f }, (Color32){ 0xdbeafeU });
        target_text(&target, 212.0f, 246.0f, "Preparing required baked resources...", (Color32){ 0xf8fafcU });
        snprintf(label, sizeof(label), "%zu / %zu", completed_count, total_required);
        target_text(&target, 212.0f, 298.0f, label, (Color32){ 0x93c5fdU });
        raylib_backend_end_frame(&game->backend);

        resource_manager_begin_frame(&game->renderer.resource_manager);
        resource_manager_process_pending_bakes(&game->renderer.resource_manager);
    }

    game->renderer.resource_manager.bake_budget_per_frame = previous_budget;
    return true;
}

static bool game_add_static_box(
    GameState* game,
    float x,
    float y,
    float half_width,
    float half_height
) {
    Entity* entity;
    TransformComponent* transform;
    RigidBodyComponent* rigid_body;
    ColliderComponent* collider;

    if (game == NULL || game->scene == NULL) {
        return false;
    }

    entity = Entity_Create(0U);
    transform = TransformComponent_Create(x, y, 0.0f);
    rigid_body = RigidBodyComponent_Create();
    collider = ColliderComponent_Create();
    if (entity == NULL || transform == NULL || rigid_body == NULL || collider == NULL) {
        if (entity != NULL) {
            Entity_Destroy(entity);
        } else {
            TransformComponent_Destroy(transform);
            RigidBodyComponent_Destroy(rigid_body);
            ColliderComponent_Destroy(collider);
        }
        return false;
    }

    rigid_body->body_type = RIGID_BODY_STATIC;
    collider->shape = COLLIDER_SHAPE_RECT;
    collider->width = half_width * 2.0f;
    collider->height = half_height * 2.0f;
    Entity_AddComponent(entity, &transform->base);
    Entity_AddComponent(entity, &rigid_body->base);
    Entity_AddComponent(entity, &collider->base);
    if (!Scene_AddEntity(game->scene, entity)) {
        Entity_Destroy(entity);
        return false;
    }

    return true;
}

static void game_scene_input(Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data) {
    GameState* game = (GameState*)user_data;
    RigidBodyComponent* rigid_body;
    float move_axis = 0.0f;
    float target_velocity_x;
    b2Vec2 velocity;

    (void)scene;

    if (game == NULL || input_state == NULL) {
        return;
    }

    rigid_body = game_get_rigid_body(game->balls[PLAYER_INDEX].entity);
    if (rigid_body == NULL || !rigid_body->has_body) {
        return;
    }

    if (input_state->buttons[ENGINE_INPUT_ACTION_LEFT]) {
        move_axis -= 1.0f;
    }
    if (input_state->buttons[ENGINE_INPUT_ACTION_RIGHT]) {
        move_axis += 1.0f;
    }

    velocity = b2Body_GetLinearVelocity(rigid_body->body_id);
    target_velocity_x = move_axis * PLAYER_MOVE_SPEED;
    velocity.x = damp_f(velocity.x, target_velocity_x, PLAYER_MOVE_SMOOTHING, dt_seconds);
    b2Body_SetLinearVelocity(rigid_body->body_id, velocity);

    if (input_state->buttons[ENGINE_INPUT_ACTION_JUMP] && game_player_is_grounded(game)) {
        b2Body_ApplyLinearImpulseToCenter(
            rigid_body->body_id,
            (b2Vec2){ 0.0f, -PLAYER_JUMP_IMPULSE },
            true
        );
    }
}

static void game_init_world(GameState *game) {
    PhysicsWorldConfig physics_config = {0};
    size_t index;

    physics_config.gravity_y = 38.0f;
    physics_config.target_hz = 60.0f;
    physics_config.max_substeps = 4U;
    physics_config.step_substep_count = 4U;
    physics_config.task_system = &game->task_system;
    game->scene = Scene_Create("Main", &physics_config);
    if (game->scene == NULL) {
        return;
    }

    game->scene->user_data = game;
    game->scene->on_input = game_scene_input;
    game->scene->view.view_width = (float)game->renderer.view_width;
    game->scene->view.view_height = (float)game->renderer.view_height;
    game->scene->view.smoothing = 0.18f;
    game->backdrop.world_width = WORLD_WIDTH;
    game->backdrop.world_height = WORLD_HEIGHT;
    game->backdrop.cell_size = GRID_CELL_SIZE;
    Scene_SetWorldBounds(game->scene, 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT);
    Scene_SetKillBounds(game->scene, 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT);

    game_add_static_box(
        game,
        WORLD_WIDTH * 0.5f,
        -WORLD_WALL_THICKNESS * 0.5f,
        WORLD_WIDTH * 0.5f,
        WORLD_WALL_THICKNESS * 0.5f
    );
    game_add_static_box(
        game,
        WORLD_WIDTH * 0.5f,
        WORLD_HEIGHT + WORLD_WALL_THICKNESS * 0.5f,
        WORLD_WIDTH * 0.5f,
        WORLD_WALL_THICKNESS * 0.5f
    );
    game_add_static_box(
        game,
        -WORLD_WALL_THICKNESS * 0.5f,
        WORLD_HEIGHT * 0.5f,
        WORLD_WALL_THICKNESS * 0.5f,
        WORLD_HEIGHT * 0.5f
    );
    game_add_static_box(
        game,
        WORLD_WIDTH + WORLD_WALL_THICKNESS * 0.5f,
        WORLD_HEIGHT * 0.5f,
        WORLD_WALL_THICKNESS * 0.5f,
        WORLD_HEIGHT * 0.5f
    );

    for (index = 0; index < (sizeof(game->balls) / sizeof(game->balls[0])); ++index) {
        Entity* entity = Entity_Create(0U);
        TransformComponent* transform;
        RigidBodyComponent* rigid_body;
        ColliderComponent* collider;
        CameraTargetComponent* camera_target = NULL;
        const float spacing_x = 42.0f;
        const float spacing_y = 34.0f;
        const size_t columns = (size_t)(WORLD_WIDTH / spacing_x) - 2U;
        float row = (float)(index / columns);
        float column = (float)(index % columns);
        float x = 52.0f + column * spacing_x;
        float y = 52.0f + row * spacing_y;

        transform = TransformComponent_Create(x, y, 0.0f);
        rigid_body = RigidBodyComponent_Create();
        collider = ColliderComponent_Create();
        if (index == PLAYER_INDEX) {
            camera_target = CameraTargetComponent_Create();
        }
        if (entity == NULL || transform == NULL || rigid_body == NULL || collider == NULL || (index == PLAYER_INDEX && camera_target == NULL)) {
            if (entity != NULL) {
                Entity_Destroy(entity);
            } else {
                TransformComponent_Destroy(transform);
                RigidBodyComponent_Destroy(rigid_body);
                ColliderComponent_Destroy(collider);
                CameraTargetComponent_Destroy(camera_target);
            }
            continue;
        }

        game->balls[index].radius = 14.0f;
        game->balls[index].entity = entity;

        rigid_body->body_type = RIGID_BODY_DYNAMIC;
        rigid_body->initial_velocity_x = ((index % 2U) == 0U) ? 4.0f : -4.0f;
        rigid_body->initial_velocity_y = ((index % 3U) == 0U) ? -2.0f : 2.0f;
        rigid_body->initial_angular_velocity = 0.45f + (float)index * 0.08f;
        rigid_body->density = 1.0f;
        rigid_body->friction = index == PLAYER_INDEX ? 0.45f : 0.18f;
        rigid_body->restitution = index == PLAYER_INDEX ? 0.25f : 0.88f;
        collider->shape = COLLIDER_SHAPE_CIRCLE;
        collider->radius = game->balls[index].radius;

        Entity_AddComponent(entity, &transform->base);
        Entity_AddComponent(entity, &rigid_body->base);
        Entity_AddComponent(entity, &collider->base);
        if (camera_target != NULL) {
            camera_target->follow = true;
            camera_target->smoothing = 0.18f;
            camera_target->lead_y = -24.0f;
            Entity_AddComponent(entity, &camera_target->base);
        }
        if (!Scene_AddEntity(game->scene, entity)) {
            Entity_Destroy(entity);
            game->balls[index].entity = NULL;
        }
    }

}

static bool game_player_is_grounded(const GameState *game) {
    RigidBodyComponent* rigid_body;

    if (game == NULL) {
        return false;
    }
    rigid_body = game_get_rigid_body(game->balls[PLAYER_INDEX].entity);
    if (rigid_body == NULL || !rigid_body->has_body) {
        return false;
    }
    return b2Body_GetContactCapacity(rigid_body->body_id) > 0;
}

static uint32_t game_step_world(GameState *game, double dt_seconds, const EngineInput *input) {
    const float fixed_dt = 1.0f / 60.0f;
    uint32_t substep_count = 0U;
    SceneInputState scene_input = {0};

    if (game == NULL || game->scene == NULL) {
        return 0U;
    }

    if (input != NULL) {
        scene_input.buttons[ENGINE_INPUT_ACTION_LEFT] = EngineInput_is_down(input, ENGINE_INPUT_ACTION_LEFT);
        scene_input.buttons[ENGINE_INPUT_ACTION_RIGHT] = EngineInput_is_down(input, ENGINE_INPUT_ACTION_RIGHT);
        scene_input.buttons[ENGINE_INPUT_ACTION_JUMP] = EngineInput_was_pressed(input, ENGINE_INPUT_ACTION_JUMP);
        scene_input.mouse_x = (float)input->mouse_x;
        scene_input.mouse_y = (float)input->mouse_y;
        scene_input.mouse_primary_down = EngineInput_is_mouse_down(input, 1U);
        scene_input.mouse_primary_pressed = EngineInput_was_mouse_pressed(input, 1U);
        scene_input.mouse_primary_released = EngineInput_was_mouse_released(input, 1U);
    }

    game->accumulator_seconds += dt_seconds;
    while (game->accumulator_seconds >= fixed_dt) {
        Scene_Update(game->scene, fixed_dt, &scene_input);
        game->accumulator_seconds -= fixed_dt;
        substep_count += 1U;
    }
    return substep_count;
}

static void game_fill_renderables(
    GameState *game,
    const uint32_t* indices,
    size_t index_count,
    SpriteRenderable *items
) {
    size_t index;

    for (index = 0; index < index_count; ++index) {
        uint32_t ball_index = indices[index];
        TransformComponent* transform = game_get_transform(game->balls[ball_index].entity);

        if (transform == NULL) {
            memset(&items[index], 0, sizeof(items[index]));
            continue;
        }

        memset(&items[index], 0, sizeof(items[index]));
        items[index].x = transform->x;
        items[index].y = transform->y;
        items[index].angle_radians = transform->angle_radians;
        items[index].anchor_x = 0.5f;
        items[index].anchor_y = 0.5f;
        items[index].layer = 0;
        items[index].visible = true;
        items[index].visual_source_handle = game->balls[ball_index].visual_source_handle;
        items[index].material_handle = game->balls[ball_index].material_handle;
        items[index].shader_handle = game->shared_ball_shader_handle;
        items[index].user_data = NULL;
    }
}

static void game_fill_debug_entities(
    GameState *game,
    const uint32_t* indices,
    size_t index_count,
    DebugEntityView *entities,
    size_t entity_capacity
) {
    size_t index;

    if (game == NULL || indices == NULL || entities == NULL) {
        return;
    }

    if (index_count > entity_capacity) {
        index_count = entity_capacity;
    }

    for (index = 0U; index < index_count; ++index) {
        uint32_t ball_index = indices[index];
        TransformComponent* transform = game_get_transform(game->balls[ball_index].entity);
        RigidBodyComponent* rigid_body = game_get_rigid_body(game->balls[ball_index].entity);
        ColliderComponent* collider = game_get_collider(game->balls[ball_index].entity);
        b2Vec2 velocity = {0};

        if (transform == NULL || rigid_body == NULL || collider == NULL || !rigid_body->has_body) {
            memset(&entities[index], 0, sizeof(entities[index]));
            continue;
        }

        velocity = b2Body_GetLinearVelocity(rigid_body->body_id);
        entities[index].id = (uint64_t)(ball_index + 1U);
        entities[index].position.x = transform->x;
        entities[index].position.y = transform->y;
        entities[index].velocity.x = velocity.x;
        entities[index].velocity.y = velocity.y;
        entities[index].angle_radians = transform->angle_radians;
        entities[index].extent_x = collider->shape == COLLIDER_SHAPE_CIRCLE ? collider->radius : collider->width * 0.5f;
        entities[index].extent_y = collider->shape == COLLIDER_SHAPE_CIRCLE ? collider->radius : collider->height * 0.5f;
        entities[index].radius = collider->radius;
        entities[index].is_circle = collider->shape == COLLIDER_SHAPE_CIRCLE;
        entities[index].is_selected = ball_index == game->selected_ball_index;
        entities[index].is_sleeping = b2Body_IsAwake(rigid_body->body_id) ? false : true;
        entities[index].is_moving = fabsf(velocity.x) + fabsf(velocity.y) > 0.05f;
    }
}

static void game_fill_single_debug_entity(
    GameState* game,
    size_t ball_index,
    DebugEntityView* entity
) {
    uint32_t id = (uint32_t)ball_index;
    game_fill_debug_entities(game, &id, 1U, entity, 1U);
}

static size_t game_pick_ball(
    GameState *game,
    Vec2 mouse_world
) {
    Entity* entity;

    if (game == NULL || game->scene == NULL) {
        return INVALID_INDEX;
    }

    entity = Scene_PickEntityAtScreen(
        game->scene,
        mouse_world.x - game->scene->view.x,
        mouse_world.y - game->scene->view.y
    );
    return game_find_ball_index(game, entity);
}

static void game_update_drag(
    GameState* game,
    const EngineInput* input,
    double dt_seconds
) {
    Vec2 mouse_world;
    size_t picked_index;

    if (game == NULL || input == NULL) {
        return;
    }

    mouse_world = camera_screen_to_world(
        &game->renderer.camera,
        (Vec2){ (float)input->mouse_x, (float)input->mouse_y }
    );

    if (EngineInput_was_mouse_pressed(input, 1U) &&
        !debug_overlay_is_pointer_over_ui(
            &game->renderer.debug_overlay,
            game->selected_ball_index != INVALID_INDEX,
            (float)input->mouse_x,
            (float)input->mouse_y,
            game->backend.window_width,
            game->backend.window_height
        )) {
        picked_index = game_pick_ball(
            game,
            mouse_world
        );
        game->selected_ball_index = picked_index;
        if (picked_index != INVALID_INDEX) {
            game->dragged_ball_index = picked_index;
            game->drag_active = true;
            game->drag_last_world = mouse_world;
            game->drag_release_velocity = (Vec2){ 0.0f, 0.0f };
        }
    }

    if (game->drag_active && game->dragged_ball_index != INVALID_INDEX) {
            Entity* entity = game->balls[game->dragged_ball_index].entity;
            RigidBodyComponent* rigid_body = game_get_rigid_body(entity);
            TransformComponent* transform = game_get_transform(entity);
            if (EngineInput_is_mouse_down(input, 1U)) {
                float dt = dt_seconds > 0.0001 ? (float)dt_seconds : (1.0f / 240.0f);
                game->drag_release_velocity.x = (mouse_world.x - game->drag_last_world.x) / dt;
                game->drag_release_velocity.y = (mouse_world.y - game->drag_last_world.y) / dt;
                PhysicsWorld_SetEntityPosition(game->scene->physics_world, entity, mouse_world.x, mouse_world.y);
                if (rigid_body != NULL && rigid_body->has_body) {
                    b2Body_SetLinearVelocity(rigid_body->body_id, (b2Vec2){ 0.0f, 0.0f });
                    b2Body_SetAngularVelocity(rigid_body->body_id, 0.0f);
                }
                if (transform != NULL) {
                    transform->dirty = false;
                }
                game->drag_last_world = mouse_world;
            } else {
                if (rigid_body != NULL && rigid_body->has_body) {
                    b2Body_SetLinearVelocity(
                        rigid_body->body_id,
                        (b2Vec2){
                            game->drag_release_velocity.x * 0.55f,
                            game->drag_release_velocity.y * 0.55f
                        }
                    );
                }
                game->drag_active = false;
                game->dragged_ball_index = INVALID_INDEX;
            }
    }
}

int JimokomiGame_Run(void) {
    GameState game;
    RuntimeConfig runtime_config;
    RendererConfig renderer_config;
    TaskSystemConfig task_system_config;
    SpriteRenderable* items = NULL;
    DebugEntityView* debug_entities = NULL;
    DebugEntityView selected_entity_view;
    size_t visible_count = 0U;
    char log_message[128];

    memset(&game, 0, sizeof(game));
    runtime_config_init_defaults(&runtime_config);
        game.tracked_ball_index = PLAYER_INDEX;
    game.selected_ball_index = INVALID_INDEX;
    game.dragged_ball_index = INVALID_INDEX;
    items = (SpriteRenderable*)calloc(BALL_COUNT, sizeof(*items));
    debug_entities = (DebugEntityView*)calloc(BALL_COUNT, sizeof(*debug_entities));
    if (items == NULL || debug_entities == NULL) {
        free(items);
        free(debug_entities);
        return 1;
    }
    if (!raylib_backend_init(
        &game.backend,
        runtime_config.window_width,
        runtime_config.window_height,
        runtime_config.window_title
    )) {
        free(items);
        free(debug_entities);
        return 1;
    }

    if (!Engine_init(&game.engine, &runtime_config.engine)) {
        raylib_backend_dispose(&game.backend);
        free(items);
        free(debug_entities);
        return 1;
    }

    memset(&task_system_config, 0, sizeof(task_system_config));
    task_system_config.requested_worker_count = 0;
    if (!task_system_init(&game.task_system, &task_system_config)) {
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        free(items);
        free(debug_entities);
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
    renderer_init(&game.renderer, &game.backend.render_backend, &renderer_config);
    if (!game_register_ball_visuals(
            &game.renderer,
            &runtime_config,
            &game.shared_ball_shader_handle,
            game.ball_source_handles,
            SOURCE_VARIANT_COUNT,
            game.ball_material_handles,
            BALL_COUNT)) {
        renderer_dispose(&game.renderer);
        task_system_dispose(&game.task_system);
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        free(items);
        free(debug_entities);
        return 1;
    }
    for (size_t i = 0U; i < BALL_COUNT; ++i) {
        game.balls[i].visual_source_handle = game.ball_source_handles[i % SOURCE_VARIANT_COUNT];
        game.balls[i].material_handle = game.ball_material_handles[i];
    }
    game_init_world(&game);
    if (game.scene == NULL) {
        renderer_dispose(&game.renderer);
        task_system_dispose(&game.task_system);
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        free(items);
        free(debug_entities);
        return 1;
    }
    game.renderer.camera.x = game.scene->view.x;
    game.renderer.camera.y = game.scene->view.y;
    (void)game_queue_required_ball_prebakes(
        &game.renderer,
        game.shared_ball_shader_handle,
        game.ball_source_handles,
        SOURCE_VARIANT_COUNT,
        game.ball_material_handles[0]
    );
    if (!game_drain_required_prebakes(&game)) {
        renderer_dispose(&game.renderer);
        Scene_Destroy(game.scene);
        task_system_dispose(&game.task_system);
        Engine_dispose(&game.engine);
        raylib_backend_dispose(&game.backend);
        free(items);
        free(debug_entities);
        return 1;
    }
    game.last_tick_ms = raylib_backend_now_ms();

    while (!raylib_backend_should_close(&game.backend)) {
        EngineInputSnapshot input_snapshot;
        DebugOverlaySnapshot debug_snapshot;
        RendererFrame frame;
        const DebugEntityView *selected_entity;
        TransformComponent* player_transform;
        RigidBodyComponent* player_body;
        uint32_t physics_substeps;
        double physics_started_ms;
        double physics_ms;
        uint64_t now_ms;
        double dt_seconds;

        raylib_backend_pump_events(&game.backend);
        if (resource_manager_get_pending_bake_count(&game.renderer.resource_manager) > 0U) {
            if (!game_drain_required_prebakes(&game)) {
                break;
            }
        }
        now_ms = raylib_backend_now_ms();
        dt_seconds = (double)(now_ms - game.last_tick_ms) / 1000.0;
        if (dt_seconds < 0.0) {
            dt_seconds = 0.0;
        } else if (dt_seconds > 0.1) {
            dt_seconds = 0.1;
        }
        game.last_tick_ms = now_ms;

        input_snapshot = raylib_backend_snapshot_input(&game.backend);
        Engine_update(&game.engine, dt_seconds, &input_snapshot);
        if (EngineInput_was_pressed(&game.engine.input, ENGINE_INPUT_ACTION_CYCLE_TARGET)) {
            game.tracked_ball_index = (game.tracked_ball_index + 1U) % BALL_COUNT;
        }
        if (EngineInput_was_pressed(&game.engine.input, ENGINE_INPUT_ACTION_DEBUG_TOGGLE)) {
            debug_overlay_toggle(&game.renderer.debug_overlay);
        }

        EngineProfiler_begin_frame(&game.engine.profiler, "game_simulation");
        EngineProfiler_set_metadata_number(&game.engine.profiler, "dt_ms", dt_seconds * 1000.0);

        physics_started_ms = game_now_ms();
        EngineProfiler_begin_scope(&game.engine.profiler, "game.physics");
        physics_substeps = game_step_world(&game, dt_seconds, &game.engine.input);
        EngineProfiler_end_scope(&game.engine.profiler, "game.physics");
        physics_ms = game_now_ms() - physics_started_ms;

        EngineProfiler_begin_scope(&game.engine.profiler, "game.debug_input");
        debug_overlay_handle_input(
            &game.renderer.debug_overlay,
            &game.engine.input,
            game.selected_ball_index != INVALID_INDEX,
            game.backend.window_width,
            game.backend.window_height
        );
        EngineProfiler_end_scope(&game.engine.profiler, "game.debug_input");

        EngineProfiler_begin_scope(&game.engine.profiler, "game.drag");
        game_update_drag(
            &game,
            &game.engine.input,
            dt_seconds
        );
        EngineProfiler_end_scope(&game.engine.profiler, "game.drag");

        player_transform = game_get_transform(game.balls[PLAYER_INDEX].entity);
        player_body = game_get_rigid_body(game.balls[PLAYER_INDEX].entity);
        game.renderer.camera.x = game.scene->view.x;
        game.renderer.camera.y = game.scene->view.y;
        game.renderer.camera.view_width = game.scene->view.view_width;
        game.renderer.camera.view_height = game.scene->view.view_height;

        visible_count = 0U;
        for (size_t i = 0U; i < BALL_COUNT; ++i) {
            TransformComponent* transform = game_get_transform(game.balls[i].entity);
            if (transform == NULL) {
                continue;
            }
            if (transform->x + game.balls[i].radius < game.renderer.camera.x ||
                transform->y + game.balls[i].radius < game.renderer.camera.y ||
                transform->x - game.balls[i].radius > game.renderer.camera.x + game.renderer.camera.view_width ||
                transform->y - game.balls[i].radius > game.renderer.camera.y + game.renderer.camera.view_height) {
                continue;
            }
            game.visible_indices[visible_count++] = (uint32_t)i;
        }

        EngineProfiler_begin_scope(&game.engine.profiler, "game.renderables");
        game_fill_renderables(&game, game.visible_indices, visible_count, items);
        EngineProfiler_end_scope(&game.engine.profiler, "game.renderables");

        EngineProfiler_begin_scope(&game.engine.profiler, "game.debug_entities");
        game_fill_debug_entities(&game, game.visible_indices, visible_count, debug_entities, BALL_COUNT);
        EngineProfiler_end_scope(&game.engine.profiler, "game.debug_entities");

        EngineProfiler_set_metadata_number(&game.engine.profiler, "ball_count", (double)BALL_COUNT);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "physics_workers", (double)task_system_get_worker_count(&game.task_system));
        EngineProfiler_set_metadata_number(&game.engine.profiler, "visible_candidates", (double)visible_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "physics_substeps", (double)physics_substeps);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "physics_ms", physics_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "tracked_ball_index", (double)game.tracked_ball_index);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "selected_ball_index", game.selected_ball_index == INVALID_INDEX ? -1.0 : (double)game.selected_ball_index);
        EngineProfiler_set_metadata_bool(&game.engine.profiler, "drag_active", game.drag_active);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "player_x", player_transform != NULL ? player_transform->x : 0.0);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "player_y", player_transform != NULL ? player_transform->y : 0.0);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "player_vx", player_body != NULL && player_body->has_body ? b2Body_GetLinearVelocity(player_body->body_id).x : 0.0);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "player_vy", player_body != NULL && player_body->has_body ? b2Body_GetLinearVelocity(player_body->body_id).y : 0.0);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "camera_x", game.scene->view.x);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "camera_y", game.scene->view.y);
        EngineProfiler_end_frame(&game.engine.profiler);

        memset(&frame, 0, sizeof(frame));
        frame.items = items;
        frame.item_count = visible_count;
        frame.backdrop_draw = game_draw_world_backdrop;
        frame.backdrop_user_data = &game.backdrop;
        frame.draw_sprites = true;
        frame.now_ms = now_ms;

        debug_snapshot.fps = (float)game.engine.stats.fps;
        debug_snapshot.update_ms = (float)game.engine.stats.last_update_ms;
        debug_snapshot.draw_ms = (float)game.engine.stats.last_draw_ms;
        debug_snapshot.physics_ms = (float)physics_ms;
        if (game.selected_ball_index != INVALID_INDEX) {
            game_fill_single_debug_entity(&game, game.selected_ball_index, &selected_entity_view);
            selected_entity = &selected_entity_view;
        } else {
            selected_entity = NULL;
        }

        Engine_draw_begin(&game.engine);
        raylib_backend_begin_frame(&game.backend, (Color32){ 0x0b1020U });
        EngineProfiler_begin_scope(&game.engine.profiler, "draw.renderer");
        renderer_draw(&game.renderer, &game.backend.render_backend, &frame);
        EngineProfiler_end_scope(&game.engine.profiler, "draw.renderer");
        EngineProfiler_begin_scope(&game.engine.profiler, "draw.debug_world");
        debug_overlay_draw_world(
            &game.renderer.debug_overlay,
            &game.backend.render_backend,
            &game.renderer.camera,
            debug_entities,
            visible_count,
            NULL,
            0U
        );
        EngineProfiler_end_scope(&game.engine.profiler, "draw.debug_world");
        EngineProfiler_begin_scope(&game.engine.profiler, "draw.debug_ui");
        debug_overlay_draw_ui(
            &game.renderer.debug_overlay,
            &game.backend.render_backend,
            &debug_snapshot,
            &game.engine.stats,
            selected_entity,
            game.backend.window_width,
            game.backend.window_height
        );
        EngineProfiler_end_scope(&game.engine.profiler, "draw.debug_ui");
        EngineProfiler_set_metadata_number(&game.engine.profiler, "world_width", WORLD_WIDTH);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "world_height", WORLD_HEIGHT);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "render_items", (double)frame.item_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "resource_visual_sources", (double)resource_manager_get_visual_source_count(&game.renderer.resource_manager));
        EngineProfiler_set_metadata_number(&game.engine.profiler, "resource_materials", (double)resource_manager_get_material_count(&game.renderer.resource_manager));
        EngineProfiler_set_metadata_number(&game.engine.profiler, "resource_shaders", (double)resource_manager_get_shader_count(&game.renderer.resource_manager));
        EngineProfiler_set_metadata_number(&game.engine.profiler, "resource_baked_surfaces", (double)resource_manager_get_baked_surface_count(&game.renderer.resource_manager));
        EngineProfiler_set_metadata_number(&game.engine.profiler, "resource_prebake_budget", (double)game.renderer.resource_manager.bake_budget_per_frame);
        EngineProfiler_set_metadata_number(
            &game.engine.profiler,
            "resource_pending_bakes",
            (double)(game.renderer.resource_manager.pending_bake_request_count - game.renderer.resource_manager.pending_bake_request_head)
        );
        EngineProfiler_set_metadata_number(
            &game.engine.profiler,
            "resource_bake_interest_entries",
            (double)game.renderer.resource_manager.bake_interest_count
        );
        EngineProfiler_set_metadata_number(
            &game.engine.profiler,
            "resource_bake_requests_this_frame",
            (double)game.renderer.resource_manager.bake_requests_this_frame
        );
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_last_items", (double)game.renderer.last_render_item_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_last_sprites", (double)game.renderer.last_sprite_draw_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_visible_items", (double)game.renderer.last_visible_item_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_overlay_draws", (double)game.renderer.last_overlay_draw_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_procedural_items", (double)game.renderer.last_procedural_item_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_sort_ms", game.renderer.last_sort_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_visibility_ms", game.renderer.last_visibility_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_body_ms", game.renderer.last_body_draw_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_overlay_ms", game.renderer.last_overlay_draw_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "debug_visible_entities", (double)game.renderer.debug_overlay.last_visible_entity_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "debug_active_collisions", (double)game.renderer.debug_overlay.last_active_collision_count);
        EngineProfiler_set_metadata_bool(&game.engine.profiler, "debug_enabled", game.renderer.debug_overlay.enabled);
        raylib_backend_end_frame(&game.backend);
        Engine_draw_end(&game.engine);
    }

    if (game.scene != NULL) {
        Scene_Destroy(game.scene);
    }
    task_system_dispose(&game.task_system);
    renderer_dispose(&game.renderer);
    Engine_dispose(&game.engine);
    raylib_backend_dispose(&game.backend);
    free(items);
    free(debug_entities);
    return 0;
}
