#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_H

#include <stdbool.h>

typedef struct ResourceManager ResourceManager;
typedef struct RenderBackend RenderBackend;

bool resource_manager_init(ResourceManager* manager, RenderBackend* backend);
void resource_manager_dispose(ResourceManager* manager);
void resource_manager_begin_frame(ResourceManager* manager);

#endif
