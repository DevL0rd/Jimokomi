#include "RandomForceComponent.h"

#include "../../Core/Memory.h"

#include <stdlib.h>

static void RandomForceComponent_DestroyBase(Component* component)
{
    RandomForceComponent_Destroy((RandomForceComponent*)component);
}

void RandomForceComponent_Init(RandomForceComponent* component)
{
    static uint32_t next_seed = 0xA341316Cu;

    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_RANDOM_FORCE, RandomForceComponent_DestroyBase);
    component->enabled = true;
    component->force_strength = 24.0f;
    component->interval_seconds = 0.08f;
    component->time_remaining_seconds = 0.0f;
    component->current_force_x = 0.0f;
    component->current_force_y = 0.0f;
    component->random_state = next_seed;
    next_seed += 0x9E3779B9u;
}

RandomForceComponent* RandomForceComponent_Create(void)
{
    RandomForceComponent* component = (RandomForceComponent*)calloc(1, sizeof(RandomForceComponent));
    if (component == NULL)
    {
        return NULL;
    }

    RandomForceComponent_Init(component);
    return component;
}

void RandomForceComponent_Destroy(RandomForceComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}
