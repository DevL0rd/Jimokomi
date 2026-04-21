#ifndef JIMOKOMI_ENGINE_APP_INTERNAL_H
#define JIMOKOMI_ENGINE_APP_INTERNAL_H

#include "App.h"
#include "Rendering/RaylibBackendInternal.h"
#include "Rendering/Renderer.h"
#include "Scene/Scene.h"
#include "Runtime/InteractionSystem.h"
#include "Core/TaskSystemInternal.h"

struct EngineAppContext
{
    Engine engine;
    RaylibBackend backend;
    Renderer* renderer;
    Scene* scene;
    TaskSystem task_system;
    InteractionSystemState interaction_state;
    uint64_t dropped_render_snapshots;
};

#endif
