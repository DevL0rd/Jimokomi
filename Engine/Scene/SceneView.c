#include "SceneView.h"

#include "SceneInternal.h"
#include "SceneLifecycleInternal.h"
#include "ScenePhysicsInternal.h"
#include "SceneSpatialInternal.h"
#include "SceneTilemapInternal.h"
#include "SpatialGrid.h"

static void Scene_UpdateBoundsFromTilemap(Scene* scene)
{
    int pixel_width = 0;
    int pixel_height = 0;

    if (scene == NULL || scene->tilemap->tilemap_adapter == NULL || scene->tilemap->tilemap == NULL || scene->lifecycle->is_overlay)
    {
        scene->spatial->view.has_world_bounds = false;
        Scene_UpdateSpatialGridBounds(scene);
        return;
    }

    if (scene->tilemap->tilemap_adapter->get_pixel_width == NULL || scene->tilemap->tilemap_adapter->get_pixel_height == NULL)
    {
        return;
    }

    pixel_width = scene->tilemap->tilemap_adapter->get_pixel_width(scene->tilemap->tilemap);
    pixel_height = scene->tilemap->tilemap_adapter->get_pixel_height(scene->tilemap->tilemap);
    if (pixel_width > 0 && pixel_height > 0)
    {
        Scene_SetWorldBounds(scene, 0.0f, 0.0f, (float)pixel_width, (float)pixel_height);
    }
}

void Scene_UpdateSpatialGridBounds(Scene* scene)
{
    Aabb bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool has_bounds = false;

    if (scene == NULL)
    {
        return;
    }

    if (scene->spatial->view.has_world_bounds)
    {
        bounds.min_x = scene->spatial->view.world_min_x;
        bounds.min_y = scene->spatial->view.world_min_y;
        bounds.max_x = scene->spatial->view.world_max_x;
        bounds.max_y = scene->spatial->view.world_max_y;
        has_bounds = true;
    }

    if (has_bounds)
    {
        SpatialGrid_SetBounds(&scene->spatial->spatial_grid, bounds);
    }
    else
    {
        SpatialGrid_ClearBounds(&scene->spatial->spatial_grid);
    }
}

void Scene_SetTilemap(Scene* scene,
                        const SceneTilemapAdapter* adapter,
                        const void* tilemap,
                        const TileRule* tile_rules,
                        size_t tile_rule_count)
{
    if (scene == NULL)
    {
        return;
    }

    scene->tilemap->tilemap_adapter = adapter;
    scene->tilemap->tilemap = tilemap;
    scene->tilemap->tile_rules = tile_rules;
    scene->tilemap->tile_rule_count = tile_rule_count;
    if (scene->physics->physics_world != NULL)
    {
        PhysicsWorld_SetTilemap(scene->physics->physics_world, adapter, tilemap, tile_rules, tile_rule_count);
    }
    Scene_UpdateBoundsFromTilemap(scene);
}

void Scene_SetWorldBounds(Scene* scene, float min_x, float min_y, float max_x, float max_y)
{
    if (scene == NULL)
    {
        return;
    }

    scene->spatial->view.has_world_bounds = true;
    scene->spatial->view.world_min_x = min_x;
    scene->spatial->view.world_min_y = min_y;
    scene->spatial->view.world_max_x = max_x;
    scene->spatial->view.world_max_y = max_y;
    Scene_UpdateSpatialGridBounds(scene);
}
