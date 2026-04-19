#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_CAMERATARGETCOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_CAMERATARGETCOMPONENT_H

#include "../Component.h"

typedef struct CameraTargetComponent
{
    Component base;
    bool follow;
    float smoothing;
    float lead_x;
    float lead_y;
} CameraTargetComponent;

void CameraTargetComponent_Init(CameraTargetComponent* component);
CameraTargetComponent* CameraTargetComponent_Create(void);
void CameraTargetComponent_Destroy(CameraTargetComponent* component);

#endif
