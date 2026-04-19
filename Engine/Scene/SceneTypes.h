#ifndef JIMOKOMI_ENGINE_SCENE_SCENETYPES_H
#define JIMOKOMI_ENGINE_SCENE_SCENETYPES_H

#include "../Physics/PhysicsWorld.h"
#include "../Rendering/RenderCommon.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct TileRule
{
    bool solid;
    bool climbable;
    uint32_t user_flags;
} TileRule;

typedef struct SceneInputState
{
    bool buttons[16];
    float mouse_x;
    float mouse_y;
    bool mouse_primary_down;
    bool mouse_primary_pressed;
    bool mouse_primary_released;
} SceneInputState;

typedef struct SceneTilemapAdapter
{
    int (*get_width_tiles)(const void* tilemap);
    int (*get_height_tiles)(const void* tilemap);
    int (*get_tile_width)(const void* tilemap);
    int (*get_tile_height)(const void* tilemap);
    int (*get_tile)(const void* tilemap, int tile_x, int tile_y, int layer_index);
    int (*get_pixel_width)(const void* tilemap);
    int (*get_pixel_height)(const void* tilemap);
} SceneTilemapAdapter;

typedef struct SceneView
{
    float x;
    float y;
    float view_width;
    float view_height;
    float smoothing;
    bool has_world_bounds;
    float world_min_x;
    float world_min_y;
    float world_max_x;
    float world_max_y;
} SceneView;

#endif
