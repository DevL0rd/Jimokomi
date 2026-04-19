#ifndef JIMOKOMI_ENGINE_SCENE_SYSTEMS_INPUTROUTINGSYSTEM_H
#define JIMOKOMI_ENGINE_SCENE_SYSTEMS_INPUTROUTINGSYSTEM_H

struct Scene;
struct SceneInputState;

void InputRoutingSystem_Update(struct Scene* scene, const struct SceneInputState* input_state, float dt_seconds);

#endif
