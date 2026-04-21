#ifndef JIMOKOMI_ENGINE_SCENE_SCENE_SPATIAL_INTERNAL_H
#define JIMOKOMI_ENGINE_SCENE_SCENE_SPATIAL_INTERNAL_H

#include "SceneInternal.h"
#include "SpatialGridInternal.h"

struct SceneSpatialState {
    SceneView view;
    SpatialGrid spatial_grid;
};

#endif
