#include "PhysicsWorldLifecycleInternal.h"
#include "PhysicsWorldTilemapInternal.h"

#include "../Scene/SceneTypes.h"

#include "../Core/Memory.h"

#include <stdlib.h>

static bool PhysicsWorld_ReserveTileBodies(PhysicsWorld* world, size_t required_capacity)
{
    b2BodyId* bodies = NULL;
    size_t new_capacity = 0;

    if (world == NULL)
    {
        return false;
    }

    if (world->tilemap->tile_body_capacity >= required_capacity)
    {
        return true;
    }

    new_capacity = world->tilemap->tile_body_capacity == 0 ? 8 : world->tilemap->tile_body_capacity;
    while (new_capacity < required_capacity)
    {
        new_capacity *= 2;
    }

    bodies = (b2BodyId*)realloc(world->tilemap->tile_bodies, sizeof(b2BodyId) * new_capacity);
    if (bodies == NULL)
    {
        return false;
    }

    world->tilemap->tile_bodies = bodies;
    world->tilemap->tile_body_capacity = new_capacity;
    return true;
}

static int PhysicsWorld_GetTileValue(const PhysicsWorld* world, int tile_x, int tile_y, int layer_index)
{
    if (world == NULL || world->tilemap->tilemap_adapter == NULL || world->tilemap->tilemap == NULL || world->tilemap->tilemap_adapter->get_tile == NULL)
    {
        return 0;
    }

    return world->tilemap->tilemap_adapter->get_tile(world->tilemap->tilemap, tile_x, tile_y, layer_index);
}

static bool PhysicsWorld_IsTileSolid(const PhysicsWorld* world, int tile_value)
{
    if (world == NULL || tile_value < 0 || (size_t)tile_value >= world->tilemap->tile_rule_count || world->tilemap->tile_rules == NULL)
    {
        return false;
    }

    return world->tilemap->tile_rules[tile_value].solid;
}

void PhysicsWorld_ClearTilemapBodies(PhysicsWorld* world)
{
    size_t index = 0;

    if (world == NULL || !world->lifecycle->has_world)
    {
        return;
    }

    for (index = 0; index < world->tilemap->tile_body_count; ++index)
    {
        if (b2Body_IsValid(world->tilemap->tile_bodies[index]))
        {
            b2DestroyBody(world->tilemap->tile_bodies[index]);
        }
    }
    world->tilemap->tile_body_count = 0;
}

static void PhysicsWorld_RebuildTilemapBodies(PhysicsWorld* world)
{
    int width_tiles = 0;
    int height_tiles = 0;
    int tile_width = 0;
    int tile_height = 0;
    int tile_y = 0;

    PhysicsWorld_ClearTilemapBodies(world);

    if (world == NULL || world->tilemap->tilemap_adapter == NULL || world->tilemap->tilemap == NULL)
    {
        return;
    }

    if (world->tilemap->tilemap_adapter->get_width_tiles == NULL || world->tilemap->tilemap_adapter->get_height_tiles == NULL ||
        world->tilemap->tilemap_adapter->get_tile_width == NULL || world->tilemap->tilemap_adapter->get_tile_height == NULL)
    {
        return;
    }

    width_tiles = world->tilemap->tilemap_adapter->get_width_tiles(world->tilemap->tilemap);
    height_tiles = world->tilemap->tilemap_adapter->get_height_tiles(world->tilemap->tilemap);
    tile_width = world->tilemap->tilemap_adapter->get_tile_width(world->tilemap->tilemap);
    tile_height = world->tilemap->tilemap_adapter->get_tile_height(world->tilemap->tilemap);

    for (tile_y = 0; tile_y < height_tiles; ++tile_y)
    {
        int tile_x = 0;
        int run_start_x = -1;
        for (tile_x = 0; tile_x <= width_tiles; ++tile_x)
        {
            int tile_value = 0;
            bool is_solid = false;
            if (tile_x < width_tiles)
            {
                tile_value = PhysicsWorld_GetTileValue(world, tile_x, tile_y, 0);
                is_solid = PhysicsWorld_IsTileSolid(world, tile_value);
            }

            if (is_solid)
            {
                if (run_start_x < 0)
                {
                    run_start_x = tile_x;
                }
            }
            else if (run_start_x >= 0)
            {
                int run_end_x = tile_x - 1;
                int run_width = (run_end_x - run_start_x) + 1;
                b2BodyDef body_def = b2DefaultBodyDef();
                b2ShapeDef shape_def = b2DefaultShapeDef();
                b2Polygon box = b2MakeBox((run_width * tile_width) * 0.5f, tile_height * 0.5f);
                b2BodyId body_id;

                body_def.type = b2_staticBody;
                body_def.position.x = (run_start_x * tile_width) + (run_width * tile_width * 0.5f);
                body_def.position.y = (tile_y * tile_height) + (tile_height * 0.5f);
                body_id = b2CreateBody(world->lifecycle->world_id, &body_def);
                shape_def.material.friction = 1.0f;
                shape_def.material.restitution = 0.0f;
                b2CreatePolygonShape(body_id, &shape_def, &box);

                if (PhysicsWorld_ReserveTileBodies(world, world->tilemap->tile_body_count + 1))
                {
                    world->tilemap->tile_bodies[world->tilemap->tile_body_count++] = body_id;
                }
                run_start_x = -1;
            }
        }
    }
}

void PhysicsWorld_SetTilemap(PhysicsWorld* world,
                               const struct SceneTilemapAdapter* adapter,
                               const void* tilemap,
                               const struct TileRule* tile_rules,
                               size_t tile_rule_count)
{
    if (world == NULL)
    {
        return;
    }

    world->tilemap->tilemap_adapter = adapter;
    world->tilemap->tilemap = tilemap;
    world->tilemap->tile_rules = tile_rules;
    world->tilemap->tile_rule_count = tile_rule_count;
    PhysicsWorld_RebuildTilemapBodies(world);
}
