#ifndef JIMOKOMI_GAME_WORLD_H
#define JIMOKOMI_GAME_WORLD_H

#include "GameState.h"

#include "../Engine/App.h"

#include <stdbool.h>

Scene* jimokomi_game_create_scene(EngineAppContext* app, JimokomiGameState* game);
bool jimokomi_game_register_resources(EngineAppContext* app, JimokomiGameState* game);
void jimokomi_game_update_sim(EngineAppContext* app, double dt_seconds, const EngineInput* input, JimokomiGameState* game);

#endif
