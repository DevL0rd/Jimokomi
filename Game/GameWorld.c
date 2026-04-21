#include "Game/GameWorld.h"

#include "BallVisualResources.h"
#include "../Engine/Scene/SceneAccess.h"
#include "../Engine/Scene/SceneFactories.h"
#include "../Engine/Scene/ScenePhysics.h"
#include "../Engine/Scene/SceneView.h"
#include "../Engine/Scene/Systems/RandomForceSystem.h"

#include <math.h>
#include <stdlib.h>

static float jimokomi_random_range(float min_value, float max_value)
{
    float t = (float)rand() / (float)RAND_MAX;
    return min_value + ((max_value - min_value) * t);
}

static void jimokomi_ball_spawn_position(float* out_x, float* out_y)
{
    if (out_x != NULL)
    {
        *out_x = jimokomi_random_range(BALL_RADIUS, WORLD_WIDTH - BALL_RADIUS);
    }
    if (out_y != NULL)
    {
        *out_y = jimokomi_random_range(BALL_RADIUS, WORLD_HEIGHT - BALL_RADIUS);
    }
}

static bool jimokomi_player_is_grounded(const JimokomiGameState* game)
{
    int contact_capacity = 0;

    if (game == NULL || game->scene == NULL || game->player == NULL)
    {
        return false;
    }

    return Scene_GetEntityContactCapacity(game->scene, game->player, &contact_capacity) &&
           contact_capacity > 0;
}

static void jimokomi_scene_input(Scene* scene, const SceneInputState* input_state, float dt_seconds, void* user_data)
{
    JimokomiGameState* game = (JimokomiGameState*)user_data;
    Vec2 velocity;
    float move_axis = 0.0f;
    float target_velocity_x;

    if (scene == NULL || game == NULL || input_state == NULL || game->player == NULL)
    {
        return;
    }

    if (input_state->buttons[ENGINE_INPUT_ACTION_LEFT])
    {
        move_axis -= 1.0f;
    }
    if (input_state->buttons[ENGINE_INPUT_ACTION_RIGHT])
    {
        move_axis += 1.0f;
    }

    if (!Scene_GetEntityLinearVelocity(scene, game->player, &velocity))
    {
        return;
    }

    target_velocity_x = move_axis * PLAYER_MOVE_SPEED;
    velocity.x = damp_f(velocity.x, target_velocity_x, PLAYER_MOVE_SMOOTHING, dt_seconds);
    Scene_SetEntityLinearVelocity(scene, game->player, velocity);

    if (input_state->buttons[ENGINE_INPUT_ACTION_JUMP] && jimokomi_player_is_grounded(game))
    {
        Scene_ApplyEntityLinearImpulse(
            scene,
            game->player,
            (Vec2){ 0.0f, -PLAYER_JUMP_IMPULSE },
            true
        );
    }
}

static Entity* jimokomi_spawn_ball(JimokomiGameState* game, size_t index)
{
    Entity* entity;
    SceneDynamicCircleDesc desc = { 0 };
    float x = 0.0f;
    float y = 0.0f;

    if (game == NULL || game->scene == NULL || index >= BALL_COUNT)
    {
        return NULL;
    }

    jimokomi_ball_spawn_position(&x, &y);
    desc.x = x;
    desc.y = y;
    desc.radius = BALL_RADIUS;
    desc.visual_source_handle = game->ball_source_handles[index % SOURCE_VARIANT_COUNT];
    desc.material_handle = game->ball_material_handle;
    desc.shader_handle = game->shared_ball_shader_handle;
    desc.initial_velocity.x = ((index % 2U) == 0U) ? 8.0f : -8.0f;
    desc.initial_velocity.y = ((index % 3U) == 0U) ? -4.0f : 4.0f;
    desc.initial_angular_velocity = 0.9f + (float)index * 0.04f;
    desc.density = 1.0f;
    desc.friction = 0.12f;
    desc.restitution = 0.0f;
    desc.selectable = true;
    desc.draggable = true;
    desc.drag_pick_radius = BALL_RADIUS;

    entity = Scene_CreateDynamicCircle(game->scene, &desc);
    if (entity == NULL)
    {
        return NULL;
    }

    RandomForceSystem_AddToEntity(game->scene, entity, BALL_RANDOM_FORCE_STRENGTH, BALL_RANDOM_FORCE_INTERVAL_SECONDS);
    game->active_ball_count += 1U;
    return entity;
}

bool jimokomi_game_register_resources(EngineAppContext* app, JimokomiGameState* game)
{
    ResourceHandle material_handles[1];
    Renderer* renderer;

    if (app == NULL || game == NULL)
    {
        return false;
    }
    renderer = EngineApp_GetRenderer(app);
    if (renderer == NULL)
    {
        return false;
    }

    if (!game_register_ball_visuals(
            renderer,
            &game->shared_ball_shader_handle,
            game->ball_source_handles,
            SOURCE_VARIANT_COUNT,
            material_handles,
            1U))
    {
        return false;
    }
    game->ball_material_handle = material_handles[0];
    return true;
}

Scene* jimokomi_game_create_scene(EngineAppContext* app, JimokomiGameState* game)
{
    PhysicsWorldConfig physics_config = { 0 };
    SceneView view = { 0 };

    if (app == NULL || game == NULL)
    {
        return NULL;
    }

    physics_config.gravity_y = PHYSICS_EARTH_GRAVITY_MPS2 * PHYSICS_PIXELS_PER_METER;
    physics_config.target_hz = 30.0f;
    physics_config.max_substeps = 8U;
    physics_config.step_substep_count = 4U;
    physics_config.task_system = EngineApp_GetTaskSystem(app);

    game->scene = Scene_Create("Main", &physics_config);
    if (game->scene == NULL)
    {
        return NULL;
    }

    Scene_SetUserData(game->scene, game);
    Scene_SetInputCallback(game->scene, jimokomi_scene_input);
    view.previous_view_width = WORLD_WIDTH;
    view.previous_view_height = WORLD_HEIGHT;
    view.view_width = WORLD_WIDTH;
    view.view_height = WORLD_HEIGHT;
    view.smoothing = 0.18f;
    Scene_SetView(game->scene, &view);
    Scene_SetWorldBounds(game->scene, 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT);
    Scene_AddBoundsColliders(game->scene, (Rect){ 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT }, WORLD_WALL_THICKNESS);
    grid_backdrop_init(&game->backdrop, WORLD_WIDTH, WORLD_HEIGHT, GRID_CELL_SIZE);

    game->player = jimokomi_spawn_ball(game, PLAYER_INDEX);
    game->spawn_cursor = 1U;
    return game->scene;
}

void jimokomi_game_update_sim(EngineAppContext* app, double dt_seconds, const EngineInput* input, JimokomiGameState* game)
{
    (void)app;
    (void)input;

    if (game == NULL || game->spawn_cursor >= BALL_COUNT)
    {
        return;
    }

    game->spawn_accumulator_seconds += dt_seconds;
    if (game->spawn_accumulator_seconds < BALL_SPAWN_INTERVAL_SECONDS)
    {
        return;
    }

    game->spawn_accumulator_seconds -= BALL_SPAWN_INTERVAL_SECONDS;
    if (jimokomi_spawn_ball(game, game->spawn_cursor) != NULL)
    {
        game->spawn_cursor += 1U;
    }
}
