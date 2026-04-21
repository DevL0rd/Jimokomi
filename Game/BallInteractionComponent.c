#include "BallInteractionComponent.h"

#include <stddef.h>
#include <stdlib.h>

static void BallInteractionComponent_DestroyBase(Component* component)
{
    BallInteractionComponent_Destroy((BallInteractionComponent*)component);
}

void BallInteractionComponent_Init(BallInteractionComponent* component, float pick_radius)
{
    if (component == NULL) {
        return;
    }

    Component_Init(&component->base, GAME_COMPONENT_BALL_INTERACTION, BallInteractionComponent_DestroyBase);
    component->draggable = true;
    component->pick_radius = pick_radius > 0.0f ? pick_radius : 1.0f;
}

BallInteractionComponent* BallInteractionComponent_Create(float pick_radius)
{
    BallInteractionComponent* component = (BallInteractionComponent*)calloc(1, sizeof(*component));
    if (component == NULL) {
        return NULL;
    }

    BallInteractionComponent_Init(component, pick_radius);
    return component;
}

void BallInteractionComponent_Destroy(BallInteractionComponent* component)
{
    if (component == NULL) {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}
