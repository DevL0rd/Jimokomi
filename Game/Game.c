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

#define BALL_COUNT 8000
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
#define SPAWN_FILL_DURATION_SECONDS 60.0

typedef struct Ball {
    Entity* entity;
    TransformComponent* transform;
    RigidBodyComponent* rigid_body;
    ColliderComponent* collider;
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
    size_t active_ball_count;
    size_t spawn_cursor;
    double spawn_elapsed_seconds;
} GameState;

static float game_lerp(float a, float b, float alpha) {
    return a + ((b - a) * alpha);
}

static float game_lerp_angle(float a, float b, float alpha) {
    float delta = fmodf(b - a, 6.28318530718f);
    if (delta > 3.14159265359f) {
        delta -= 6.28318530718f;
    } else if (delta < -3.14159265359f) {
        delta += 6.28318530718f;
    }
    return a + delta * alpha;
}

static bool game_player_is_grounded(const GameState* game);

static void game_ball_spawn_position(size_t index, float* out_x, float* out_y) {
    const float ball_diameter = 14.0f;
    const float margin_x = 40.0f;
    const float margin_y = 40.0f;
    const float usable_width = WORLD_WIDTH - margin_x * 2.0f;
    const float usable_height = WORLD_HEIGHT - margin_y * 2.0f;
    const size_t columns = (size_t)(usable_width / ball_diameter);
    const size_t rows = (BALL_COUNT + columns - 1U) / columns;
    const float spacing_x = columns > 1U ? usable_width / (float)(columns - 1U) : 0.0f;
    const float spacing_y = rows > 1U ? usable_height / (float)(rows - 1U) : 0.0f;
    const float row = (float)(index / columns);
    const float column = (float)(index % columns);

    if (out_x != NULL) {
        *out_x = margin_x + column * spacing_x;
    }
    if (out_y != NULL) {
        *out_y = margin_y + row * spacing_y;
    }
}

static bool game_spawn_ball(GameState* game, size_t index) {
    Entity* entity;
    TransformComponent* transform;
    RigidBodyComponent* rigid_body;
    ColliderComponent* collider;
    CameraTargetComponent* camera_target = NULL;
    float x = 0.0f;
    float y = 0.0f;

    if (game == NULL || game->scene == NULL || index >= BALL_COUNT || game->balls[index].entity != NULL) {
        return false;
    }

    game_ball_spawn_position(index, &x, &y);
    entity = Entity_Create(0U);
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
        return false;
    }

    game->balls[index].radius = 7.0f;
    game->balls[index].entity = entity;
    game->balls[index].transform = transform;
    game->balls[index].rigid_body = rigid_body;
    game->balls[index].collider = collider;

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
        game->balls[index].transform = NULL;
        game->balls[index].rigid_body = NULL;
        game->balls[index].collider = NULL;
        return false;
    }

    game->active_ball_count += 1U;
    return true;
}

static void game_update_spawn_curve(GameState* game, double dt_seconds) {
    double progress;
    size_t target_active_count;

    if (game == NULL || game->spawn_cursor >= BALL_COUNT) {
        return;
    }

    game->spawn_elapsed_seconds += dt_seconds;
    progress = game->spawn_elapsed_seconds / SPAWN_FILL_DURATION_SECONDS;
    if (progress < 0.0) {
        progress = 0.0;
    } else if (progress > 1.0) {
        progress = 1.0;
    }

    target_active_count = (size_t)((double)BALL_COUNT * progress);
    if (target_active_count < 1U) {
        target_active_count = 1U;
    }

    while (game->active_ball_count < target_active_count && game->spawn_cursor < BALL_COUNT) {
        if (game_spawn_ball(game, game->spawn_cursor)) {
            game->spawn_cursor += 1U;
        } else {
            break;
        }
    }
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

    rigid_body = game->balls[PLAYER_INDEX].rigid_body;
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
    static const float adaptive_levels[] = { 60.0f, 45.0f, 30.0f, 20.0f, 15.0f, 10.0f, 8.0f };

    physics_config.gravity_y = 38.0f;
    physics_config.target_hz = 60.0f;
    physics_config.adaptive_enabled = true;
    physics_config.adaptive_levels = adaptive_levels;
    physics_config.adaptive_level_count = sizeof(adaptive_levels) / sizeof(adaptive_levels[0]);
    physics_config.downshift_frame_ms = 12.0f;
    physics_config.upshift_frame_ms = 6.0f;
    physics_config.tuner_hold_frames = 8U;
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
    game->scene->kill_plane_enabled = false;

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
    game_spawn_ball(game, PLAYER_INDEX);
    game->spawn_cursor = 1U;
}

static bool game_player_is_grounded(const GameState *game) {
    RigidBodyComponent* rigid_body;

    if (game == NULL) {
        return false;
    }
    rigid_body = game->balls[PLAYER_INDEX].rigid_body;
    if (rigid_body == NULL || !rigid_body->has_body) {
        return false;
    }
    return b2Body_GetContactCapacity(rigid_body->body_id) > 0;
}

static uint32_t game_step_world(GameState *game, double dt_seconds, const EngineInput *input) {
    float fixed_dt = 1.0f / 60.0f;
    uint32_t substep_count = 0U;
    SceneInputState scene_input = {0};

    if (game == NULL || game->scene == NULL) {
        return 0U;
    }

    if (game->scene->physics_world != NULL) {
        PhysicsWorldSnapshot physics_snapshot;
        PhysicsWorld_GetSnapshot(game->scene->physics_world, &physics_snapshot);
        if (physics_snapshot.physics_fixed_dt > 0.0f) {
            fixed_dt = physics_snapshot.physics_fixed_dt;
        }
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

static bool game_fill_single_debug_entity(
    const GameState* game,
    size_t ball_index,
    DebugEntityView* entity,
    float render_alpha
) {
    const Ball* ball;
    b2Vec2 velocity;

    if (game == NULL || entity == NULL || ball_index >= BALL_COUNT) {
        return false;
    }

    ball = &game->balls[ball_index];
    if (ball->transform == NULL || ball->rigid_body == NULL || ball->collider == NULL || !ball->rigid_body->has_body) {
        memset(entity, 0, sizeof(*entity));
        return false;
    }

    velocity = b2Body_GetLinearVelocity(ball->rigid_body->body_id);
    memset(entity, 0, sizeof(*entity));
    entity->id = (uint64_t)(ball_index + 1U);
    entity->position.x = game_lerp(ball->transform->previous_x, ball->transform->x, render_alpha);
    entity->position.y = game_lerp(ball->transform->previous_y, ball->transform->y, render_alpha);
    entity->velocity.x = velocity.x;
    entity->velocity.y = velocity.y;
    entity->angle_radians = game_lerp_angle(
        ball->transform->previous_angle_radians,
        ball->transform->angle_radians,
        render_alpha
    );
    entity->extent_x = ball->collider->shape == COLLIDER_SHAPE_CIRCLE ? ball->collider->radius : ball->collider->width * 0.5f;
    entity->extent_y = ball->collider->shape == COLLIDER_SHAPE_CIRCLE ? ball->collider->radius : ball->collider->height * 0.5f;
    entity->radius = ball->collider->radius;
    entity->is_circle = ball->collider->shape == COLLIDER_SHAPE_CIRCLE;
    entity->is_selected = ball_index == game->selected_ball_index;
    entity->is_sleeping = !b2Body_IsAwake(ball->rigid_body->body_id);
    entity->is_moving = fabsf(velocity.x) + fabsf(velocity.y) > 0.05f;
    return true;
}

static size_t game_collect_visible_frame_data(
    GameState *game,
    SpriteRenderable *items,
    DebugEntityView *entities,
    size_t item_capacity,
    float render_alpha
) {
    size_t index;
    size_t visible_count = 0U;
    bool collect_debug = false;
    const float camera_x = game != NULL ? game->renderer.camera.x : 0.0f;
    const float camera_y = game != NULL ? game->renderer.camera.y : 0.0f;
    const float camera_right = game != NULL ? game->renderer.camera.x + game->renderer.camera.view_width : 0.0f;
    const float camera_bottom = game != NULL ? game->renderer.camera.y + game->renderer.camera.view_height : 0.0f;

    if (game == NULL || items == NULL) {
        return 0U;
    }

    collect_debug = entities != NULL &&
        game->renderer.debug_overlay.enabled &&
        game->renderer.debug_overlay.draw_world_gizmos;

    for (index = 0U; index < BALL_COUNT && visible_count < item_capacity; ++index) {
        Ball* ball = &game->balls[index];
        TransformComponent* transform = ball->transform;
        SpriteRenderable* item;

        if (transform == NULL) {
            continue;
        }
        if (transform->x + ball->radius < camera_x ||
            transform->y + ball->radius < camera_y ||
            transform->x - ball->radius > camera_right ||
            transform->y - ball->radius > camera_bottom) {
            continue;
        }

        item = &items[visible_count];
        item->x = game_lerp(transform->previous_x, transform->x, render_alpha);
        item->y = game_lerp(transform->previous_y, transform->y, render_alpha);
        item->angle_radians = game_lerp_angle(
            transform->previous_angle_radians,
            transform->angle_radians,
            render_alpha
        );
        item->anchor_x = 0.5f;
        item->anchor_y = 0.5f;
        item->layer = 0;
        item->visible = true;
        item->visual_source_handle = ball->visual_source_handle;
        item->material_handle = ball->material_handle;
        item->shader_handle = game->shared_ball_shader_handle;
        item->user_data = NULL;

        if (collect_debug) {
            b2Vec2 velocity = { 0 };

            memset(&entities[visible_count], 0, sizeof(entities[visible_count]));
            if (ball->rigid_body != NULL && ball->collider != NULL && ball->rigid_body->has_body) {
                velocity = b2Body_GetLinearVelocity(ball->rigid_body->body_id);
                entities[visible_count].id = (uint64_t)(index + 1U);
                entities[visible_count].position.x = item->x;
                entities[visible_count].position.y = item->y;
                entities[visible_count].velocity.x = velocity.x;
                entities[visible_count].velocity.y = velocity.y;
                entities[visible_count].angle_radians = item->angle_radians;
                entities[visible_count].extent_x = ball->collider->shape == COLLIDER_SHAPE_CIRCLE ? ball->collider->radius : ball->collider->width * 0.5f;
                entities[visible_count].extent_y = ball->collider->shape == COLLIDER_SHAPE_CIRCLE ? ball->collider->radius : ball->collider->height * 0.5f;
                entities[visible_count].radius = ball->collider->radius;
                entities[visible_count].is_circle = ball->collider->shape == COLLIDER_SHAPE_CIRCLE;
                entities[visible_count].is_selected = index == game->selected_ball_index;
                entities[visible_count].is_sleeping = !b2Body_IsAwake(ball->rigid_body->body_id);
                entities[visible_count].is_moving = fabsf(velocity.x) + fabsf(velocity.y) > 0.05f;
            }
        }

        visible_count += 1U;
    }

    return visible_count;
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
            RigidBodyComponent* rigid_body = game->balls[game->dragged_ball_index].rigid_body;
            TransformComponent* transform = game->balls[game->dragged_ball_index].transform;
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
    game.active_ball_count = 0U;
    game.spawn_cursor = 0U;
    game.spawn_elapsed_seconds = 0.0;
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
        float render_alpha;

        raylib_backend_pump_events(&game.backend);
        now_ms = raylib_backend_now_ms();
        dt_seconds = (double)(now_ms - game.last_tick_ms) / 1000.0;
        if (dt_seconds < 0.0) {
            dt_seconds = 0.0;
        } else if (dt_seconds > 0.1) {
            dt_seconds = 0.1;
        }
        game.last_tick_ms = now_ms;
        game_update_spawn_curve(&game, dt_seconds);

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
        Scene_UpdatePerformanceBudget(game.scene, (float)physics_ms);
        render_alpha = 1.0f;

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

        player_transform = game.balls[PLAYER_INDEX].transform;
        player_body = game.balls[PLAYER_INDEX].rigid_body;
        game.renderer.camera.x = game_lerp(game.scene->view.previous_x, game.scene->view.x, render_alpha);
        game.renderer.camera.y = game_lerp(game.scene->view.previous_y, game.scene->view.y, render_alpha);
        game.renderer.camera.view_width = game.scene->view.view_width;
        game.renderer.camera.view_height = game.scene->view.view_height;

        EngineProfiler_begin_scope(&game.engine.profiler, "game.renderables");
        visible_count = game_collect_visible_frame_data(&game, items, debug_entities, BALL_COUNT, render_alpha);
        EngineProfiler_end_scope(&game.engine.profiler, "game.renderables");

        EngineProfiler_set_metadata_number(&game.engine.profiler, "ball_count", (double)BALL_COUNT);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "physics_workers", (double)task_system_get_worker_count(&game.task_system));
        if (game.scene != NULL && game.scene->physics_world != NULL) {
            PhysicsWorldSnapshot physics_snapshot;
            PhysicsWorld_GetSnapshot(game.scene->physics_world, &physics_snapshot);
            EngineProfiler_set_metadata_number(&game.engine.profiler, "physics_hz", physics_snapshot.physics_hz);
            EngineProfiler_set_metadata_number(&game.engine.profiler, "physics_step_substeps", (double)physics_snapshot.physics_step_substeps);
            EngineProfiler_set_metadata_number(&game.engine.profiler, "physics_tuner_level", (double)physics_snapshot.tuner_level_index);
        }
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
            selected_entity = game_fill_single_debug_entity(&game, game.selected_ball_index, &selected_entity_view, render_alpha)
                ? &selected_entity_view
                : NULL;
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
            frame.now_ms,
            &game.renderer.camera,
            game.renderer.debug_overlay.enabled && game.renderer.debug_overlay.draw_world_gizmos ? debug_entities : NULL,
            game.renderer.debug_overlay.enabled && game.renderer.debug_overlay.draw_world_gizmos ? visible_count : 0U,
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
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_instanced_batches", (double)game.renderer.last_instanced_batch_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_instanced_draws", (double)game.renderer.last_instanced_draw_count);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_sort_ms", game.renderer.last_sort_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_visibility_ms", game.renderer.last_visibility_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_body_ms", game.renderer.last_body_draw_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_overlay_ms", game.renderer.last_overlay_draw_ms);
        EngineProfiler_set_metadata_number(&game.engine.profiler, "renderer_instance_ms", game.renderer.last_instance_draw_ms);
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
