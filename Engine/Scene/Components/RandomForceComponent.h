#ifndef JIMOKOMI_ENGINE_SCENE_COMPONENTS_RANDOMFORCECOMPONENT_H
#define JIMOKOMI_ENGINE_SCENE_COMPONENTS_RANDOMFORCECOMPONENT_H

#include "../Component.h"

#include <stdint.h>

typedef struct RandomForceComponent
{
    Component base;
    bool enabled;
    float force_strength;
    float interval_seconds;
    float time_remaining_seconds;
    float current_force_x;
    float current_force_y;
    uint32_t random_state;
} RandomForceComponent;

void RandomForceComponent_Init(RandomForceComponent* component);
RandomForceComponent* RandomForceComponent_Create(void);
void RandomForceComponent_Destroy(RandomForceComponent* component);

#endif
