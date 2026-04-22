#include "Game/Game.h"
#include "Game/GameWorld.h"

#include "../Engine/App.h"
#include "../Engine/Rendering/GridBackdrop.h"

#include <string.h>

static bool jimokomi_register_resources_callback(EngineAppContext* app, void* user_data)
{
    return jimokomi_game_register_resources(app, (JimokomiGameState*)user_data);
}

static Scene* jimokomi_create_scene_callback(EngineAppContext* app, void* user_data)
{
    return jimokomi_game_create_scene(app, (JimokomiGameState*)user_data);
}

static void jimokomi_update_sim_callback(EngineAppContext* app, double dt_seconds, const EngineInput* input, void* user_data)
{
    jimokomi_game_update_sim(app, dt_seconds, input, (JimokomiGameState*)user_data);
}

static void jimokomi_post_update_sim_callback(EngineAppContext* app, double dt_seconds, const EngineInput* input, void* user_data)
{
    jimokomi_game_post_update_sim(app, dt_seconds, input, (JimokomiGameState*)user_data);
}

int JimokomiGame_Run(void)
{
    JimokomiGameState game;
    EngineAppDesc app_desc;
    int exit_code;

    memset(&game, 0, sizeof(game));
    EngineAppDesc_InitDefaults(&app_desc);
    app_desc.user_data = &game;
    app_desc.backdrop_draw = grid_backdrop_draw;
    app_desc.backdrop_user_data = &game.backdrop;
    app_desc.backdrop_signature = grid_backdrop_signature_from_user_data;
    app_desc.register_resources = jimokomi_register_resources_callback;
    app_desc.create_scene = jimokomi_create_scene_callback;
    app_desc.update_sim = jimokomi_update_sim_callback;
    app_desc.post_update_sim = jimokomi_post_update_sim_callback;
    exit_code = EngineApp_Run(&app_desc);
    game_liquid_sources_dispose(&game.liquid_sources);
    return exit_code;
}
