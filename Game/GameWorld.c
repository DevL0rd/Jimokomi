#include "Game/GameInternal.h"

#include <math.h>

static void game_ball_spawn_position(const GameState* game, size_t index, float* out_x, float* out_y) {
    float min_x = BALL_RADIUS;
    float max_x = WORLD_WIDTH - BALL_RADIUS;
    float min_y = BALL_RADIUS;
    float max_y = WORLD_HEIGHT - BALL_RADIUS;
    (void)game;
    (void)index;

    if (out_x == NULL && out_y == NULL) {
        return;
    }

    if (out_x != NULL) {
        *out_x = game_random_range(min_x, max_x);
    }
    if (out_y != NULL) {
        *out_y = game_random_range(min_y, max_y);
    }
}

size_t game_query_player_neighborhood(GameState* game, float radius) {
    Entity* nearby_entities[256];
    Vec2 center = { 0.0f, 0.0f };

    if (game == NULL || game->scene == NULL || game->balls[PLAYER_INDEX].transform == NULL) {
        return 0U;
    }

    center.x = game->balls[PLAYER_INDEX].transform->x;
    center.y = game->balls[PLAYER_INDEX].transform->y;
    return Scene_QueryEntitiesInRadius(game->scene, center, radius, nearby_entities, 256U);
}

static bool game_spawn_ball(GameState* game, size_t index) {
    Entity* entity;
    TransformComponent* transform;
    RigidBodyComponent* rigid_body;
    ColliderComponent* collider;
    BallInteractionComponent* interaction;
    RandomForceComponent* random_force;
    float x = 0.0f;
    float y = 0.0f;

    if (game == NULL || game->scene == NULL || index >= BALL_COUNT || game->balls[index].entity != NULL) {
        return false;
    }

    game_ball_spawn_position(game, index, &x, &y);
    game->last_spawn_region_dirty_x = game_world_cell_x(x);
    game->last_spawn_region_dirty_y = game_world_cell_y(y);
    game->spawn_region_dirty_count += 1U;
    entity = Entity_Create(0U);
    transform = TransformComponent_Create(x, y, 0.0f);
    rigid_body = RigidBodyComponent_Create();
    collider = ColliderComponent_Create();
    interaction = BallInteractionComponent_Create(BALL_RADIUS);
    random_force = RandomForceComponent_Create();
    if (entity == NULL || transform == NULL || rigid_body == NULL || collider == NULL || interaction == NULL || random_force == NULL) {
        if (entity != NULL) {
            Entity_Destroy(entity);
        } else {
            TransformComponent_Destroy(transform);
            RigidBodyComponent_Destroy(rigid_body);
            ColliderComponent_Destroy(collider);
            BallInteractionComponent_Destroy(interaction);
            RandomForceComponent_Destroy(random_force);
        }
        return false;
    }

    game->balls[index].radius = BALL_RADIUS;
    game->balls[index].entity = entity;
    game->balls[index].transform = transform;
    game->balls[index].rigid_body = rigid_body;
    game->balls[index].collider = collider;
    game->balls[index].interaction = interaction;

    rigid_body->body_type = RIGID_BODY_DYNAMIC;
    rigid_body->initial_velocity_x = ((index % 2U) == 0U) ? 8.0f : -8.0f;
    rigid_body->initial_velocity_y = ((index % 3U) == 0U) ? -4.0f : 4.0f;
    rigid_body->initial_angular_velocity = 0.9f + (float)index * 0.04f;
    rigid_body->density = 1.0f;
    rigid_body->friction = 0.12f;
    rigid_body->restitution = 0.0f;
    RigidBodyComponent_MarkDefinitionDirty(rigid_body);
    ColliderComponent_SetCircle(collider, game->balls[index].radius);
    random_force->force_strength = BALL_RANDOM_FORCE_STRENGTH;
    random_force->interval_seconds = BALL_RANDOM_FORCE_INTERVAL_SECONDS;

    Entity_AddComponent(entity, &transform->base);
    Entity_AddComponent(entity, &rigid_body->base);
    Entity_AddComponent(entity, &collider->base);
    Entity_AddComponent(entity, &interaction->base);
    Entity_AddComponent(entity, &random_force->base);
    Entity_SetUserData(entity, &game->balls[index]);
    game_mark_ball_visual_state_dirty(&game->balls[index]);
    game_mark_ball_highlight_state(&game->balls[index], false, false);
    if (!Scene_AddEntity(game->scene, entity)) {
        Entity_Destroy(entity);
        game->balls[index].entity = NULL;
        game->balls[index].transform = NULL;
        game->balls[index].rigid_body = NULL;
        game->balls[index].collider = NULL;
        game->balls[index].interaction = NULL;
        return false;
    }

    game->active_ball_count += 1U;
    return true;
}

void game_update_spawn_curve(GameState* game, double dt_seconds) {
    if (game == NULL || game->spawn_cursor >= BALL_COUNT) {
        return;
    }

    game->spawn_accumulator_seconds += dt_seconds;
    if (game->spawn_accumulator_seconds < BALL_SPAWN_INTERVAL_SECONDS) {
        return;
    }

    game->spawn_accumulator_seconds -= BALL_SPAWN_INTERVAL_SECONDS;
    if (game_spawn_ball(game, game->spawn_cursor)) {
        game->spawn_cursor += 1U;
    }
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
    RigidBodyComponent_MarkDefinitionDirty(rigid_body);
    ColliderComponent_SetRect(collider, half_width * 2.0f, half_height * 2.0f);
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
    PhysicsWorld* physics_world = Scene_GetPhysicsWorld(scene);
    RigidBodyComponent* rigid_body;
    Vec2 velocity;
    float move_axis = 0.0f;
    float target_velocity_x;

    if (scene == NULL || game == NULL || input_state == NULL || physics_world == NULL) {
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

    if (!PhysicsWorld_GetEntityLinearVelocity(physics_world, game->balls[PLAYER_INDEX].entity, &velocity)) {
        return;
    }
    target_velocity_x = move_axis * PLAYER_MOVE_SPEED;
    velocity.x = damp_f(velocity.x, target_velocity_x, PLAYER_MOVE_SMOOTHING, dt_seconds);
    PhysicsWorld_SetEntityLinearVelocity(physics_world, game->balls[PLAYER_INDEX].entity, velocity);

    if (input_state->buttons[ENGINE_INPUT_ACTION_JUMP] && game_player_is_grounded(game)) {
        PhysicsWorld_ApplyEntityLinearImpulse(
            physics_world,
            game->balls[PLAYER_INDEX].entity,
            (Vec2){ 0.0f, -PLAYER_JUMP_IMPULSE },
            true);
    }
}

void game_init_world(GameState *game) {
    PhysicsWorldConfig physics_config = {0};
    SceneView view = { 0 };
    static const float adaptive_levels[] = { 120.0f, 90.0f, 75.0f, 60.0f, 45.0f, 30.0f, 20.0f, 4.0f };

    physics_config.gravity_y = 9.81f;
    physics_config.target_hz = 120.0f;
    physics_config.adaptive_enabled = true;
    physics_config.adaptive_levels = adaptive_levels;
    physics_config.adaptive_level_count = sizeof(adaptive_levels) / sizeof(adaptive_levels[0]);
    physics_config.downshift_frame_ms = 18.0f;
    physics_config.upshift_frame_ms = 14.5f;
    physics_config.max_substeps = 8U;
    physics_config.step_substep_count = 4U;
    physics_config.task_system = &game->task_system;
    game->sim_step_dt_seconds = 1.0 / (double)physics_config.target_hz;
    game->scene = Scene_Create("Main", &physics_config);
    if (game->scene == NULL) {
        return;
    }

    game->cached_sim_fixed_dt_seconds = game->sim_step_dt_seconds;
    game->cached_sim_max_substeps = physics_config.max_substeps;
    game_refresh_sim_step_config(game);
    Scene_SetUserData(game->scene, game);
    Scene_SetInputCallback(game->scene, game_scene_input);
    view.previous_view_width = WORLD_WIDTH;
    view.previous_view_height = WORLD_HEIGHT;
    view.view_width = WORLD_WIDTH;
    view.view_height = WORLD_HEIGHT;
    view.smoothing = 0.18f;
    Scene_SetView(game->scene, &view);
    game->backdrop.world_width = WORLD_WIDTH;
    game->backdrop.world_height = WORLD_HEIGHT;
    game->backdrop.cell_size = GRID_CELL_SIZE;
    game_mark_backdrop_dirty(&game->backdrop);
    Scene_SetWorldBounds(game->scene, 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT);

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

bool game_player_is_grounded(const GameState *game) {
    PhysicsWorld* physics_world = NULL;
    RigidBodyComponent* rigid_body;
    int contact_capacity = 0;

    if (game == NULL || game->scene == NULL) {
        return false;
    }
    physics_world = Scene_GetPhysicsWorld(game->scene);
    if (physics_world == NULL) {
        return false;
    }
    rigid_body = game->balls[PLAYER_INDEX].rigid_body;
    if (rigid_body == NULL || !rigid_body->has_body) {
        return false;
    }
    return PhysicsWorld_GetEntityContactCapacity(
        physics_world,
        game->balls[PLAYER_INDEX].entity,
        &contact_capacity) && contact_capacity > 0;
}

void game_refresh_sim_step_config(GameState* game) {
    PhysicsWorld* physics_world = NULL;
    float fixed_dt = 0.0f;
    uint32_t max_substeps = 0U;

    if (game == NULL || game->scene == NULL) {
        return;
    }
    physics_world = Scene_GetPhysicsWorld(game->scene);
    if (physics_world == NULL) {
        return;
    }

    PhysicsWorld_GetStepConfig(physics_world, &fixed_dt, &max_substeps);
    game->cached_sim_fixed_dt_seconds = fixed_dt > 0.0f ? (double)fixed_dt : 1.0 / 60.0;
    game->cached_sim_max_substeps = max_substeps > 0U ? max_substeps : 4U;
    if (game->sim_step_dt_seconds <= 0.0 || game->accumulator_seconds <= game->sim_step_dt_seconds) {
        game->sim_step_dt_seconds = game->cached_sim_fixed_dt_seconds;
    }
}

uint32_t game_step_world(GameState *game, double dt_seconds, const EngineInput *input) {
    double fixed_dt = 1.0 / 60.0;
    uint32_t max_substeps = 4U;
    SceneInputState scene_input = {0};

    if (game == NULL || game->scene == NULL) {
        return 0U;
    }

    fixed_dt = game->cached_sim_fixed_dt_seconds > 0.0 ? game->cached_sim_fixed_dt_seconds : game->sim_step_dt_seconds;
    fixed_dt = fixed_dt > 0.0 ? fixed_dt : 1.0 / 60.0;
    max_substeps = game->cached_sim_max_substeps > 0U ? game->cached_sim_max_substeps : 4U;

    game->accumulator_seconds = fmin(
        game->accumulator_seconds + dt_seconds,
        fixed_dt * (double)max_substeps
    );

    if (game->accumulator_seconds < fixed_dt) {
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

    Scene_Update(game->scene, (float)fixed_dt, &scene_input);
    game->accumulator_seconds -= fixed_dt;
    return 1U;
}
