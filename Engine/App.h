#ifndef JIMOKOMI_ENGINE_APP_H
#define JIMOKOMI_ENGINE_APP_H

#include "Engine.h"
#include "Core/Input.h"
#include "Rendering/RenderTypes.h"

typedef struct EngineAppContext EngineAppContext;
typedef struct Renderer Renderer;
typedef struct Scene Scene;
typedef struct TaskSystem TaskSystem;

typedef bool (*EngineAppRegisterResourcesFn)(EngineAppContext* app, void* user_data);
typedef Scene* (*EngineAppCreateSceneFn)(EngineAppContext* app, void* user_data);
typedef void (*EngineAppSimUpdateFn)(EngineAppContext* app, double dt_seconds, const EngineInput* input, void* user_data);

typedef struct EngineAppDesc
{
    void* user_data;
    RendererBackdropDrawFn backdrop_draw;
    void* backdrop_user_data;
    uint64_t (*backdrop_signature)(void* user_data);
    EngineAppRegisterResourcesFn register_resources;
    EngineAppCreateSceneFn create_scene;
    EngineAppSimUpdateFn update_sim;
} EngineAppDesc;

void EngineAppDesc_InitDefaults(EngineAppDesc* desc);
int EngineApp_Run(const EngineAppDesc* desc);
Engine* EngineApp_GetEngine(EngineAppContext* app);
Renderer* EngineApp_GetRenderer(EngineAppContext* app);
Scene* EngineApp_GetScene(EngineAppContext* app);
TaskSystem* EngineApp_GetTaskSystem(EngineAppContext* app);

#endif
