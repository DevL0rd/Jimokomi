#include "CameraTargetComponent.h"

#include "../../Core/Memory.h"

#include <stdlib.h>

static void CameraTargetComponent_DestroyBase(Component* component)
{
    CameraTargetComponent_Destroy((CameraTargetComponent*)component);
}

void CameraTargetComponent_Init(CameraTargetComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Init(&component->base, COMPONENT_CAMERA_TARGET, CameraTargetComponent_DestroyBase);
    component->follow = true;
    component->smoothing = 0.0f;
    component->lead_x = 0.0f;
    component->lead_y = 0.0f;
}

CameraTargetComponent* CameraTargetComponent_Create(void)
{
    CameraTargetComponent* component = (CameraTargetComponent*)calloc(1, sizeof(CameraTargetComponent));
    if (component == NULL)
    {
        return NULL;
    }

    CameraTargetComponent_Init(component);
    return component;
}

void CameraTargetComponent_Destroy(CameraTargetComponent* component)
{
    if (component == NULL)
    {
        return;
    }

    Component_Dispose(&component->base);
    free(component);
}
