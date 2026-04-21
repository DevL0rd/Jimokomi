#ifndef JIMOKOMI_GAME_BALLINTERACTIONCOMPONENT_H
#define JIMOKOMI_GAME_BALLINTERACTIONCOMPONENT_H

#include "../Engine/Scene/Component.h"

#include <stdbool.h>

#define GAME_COMPONENT_BALL_INTERACTION ((ComponentType)(COMPONENT_CUSTOM_BASE + 1))

typedef struct BallInteractionComponent
{
    Component base;
    bool draggable;
    float pick_radius;
} BallInteractionComponent;

void BallInteractionComponent_Init(BallInteractionComponent* component, float pick_radius);
BallInteractionComponent* BallInteractionComponent_Create(float pick_radius);
void BallInteractionComponent_Destroy(BallInteractionComponent* component);

#endif
