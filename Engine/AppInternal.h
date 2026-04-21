#ifndef JIMOKOMI_ENGINE_APP_INTERNAL_H
#define JIMOKOMI_ENGINE_APP_INTERNAL_H

#include "App.h"
#include "Rendering/RaylibBackend.h"
#include "Rendering/Renderer.h"
#include "Rendering/ResourceCommandQueue.h"
#include "Scene/Scene.h"
#include "Runtime/InteractionSystem.h"
#include "Core/TaskSystem.h"

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

#endif
