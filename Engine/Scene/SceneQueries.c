#include "SceneInternal.h"

#include "EntityInternal.h"
#include "SpatialGrid.h"
#include "Components/ColliderComponent.h"
#include "Components/TransformComponent.h"

#include <stdlib.h>

struct Entity* Scene_PickEntityAtScreen(Scene* scene, float screen_x, float screen_y)
{
    PhysicsQueryResult* hits = NULL;
    struct Entity** query_entities = NULL;
    size_t entity_count;
    size_t hit_count = 0;
    size_t index = 0;
    float world_x = 0.0f;
    float world_y = 0.0f;

    if (scene == NULL || scene->is_overlay)
    {
        return NULL;
    }

    entity_count = Scene_GetEntityCount(scene);
    if (entity_count == 0U)
    {
        return NULL;
    }

    world_x = scene->view.x + screen_x;
    world_y = scene->view.y + screen_y;

    hits = (PhysicsQueryResult*)calloc(entity_count, sizeof(*hits));
    if (hits == NULL)
    {
        return NULL;
    }

    hit_count = PhysicsWorld_QueryPoint(scene->physics_world, world_x, world_y, hits, entity_count);
    while (hit_count > 0)
    {
        if (Entity_IsActive(hits[hit_count - 1].entity))
        {
            struct Entity* picked_entity = hits[hit_count - 1].entity;
            free(hits);
            return picked_entity;
        }
        --hit_count;
    }
    free(hits);

    {
        Aabb query_bounds = { world_x - 0.5f, world_y - 0.5f, world_x + 0.5f, world_y + 0.5f };
        size_t query_count;

        query_entities = (struct Entity**)calloc(entity_count, sizeof(*query_entities));
        if (query_entities == NULL)
        {
            return NULL;
        }

        query_count = Scene_QueryEntitiesInAabb(scene, query_bounds, query_entities, entity_count);
        for (index = query_count; index > 0; --index)
        {
            struct Entity* entity = query_entities[index - 1];
            TransformComponent* transform = NULL;
            ColliderComponent* collider = NULL;

            if (!Entity_IsActive(entity))
            {
                continue;
            }

            transform = (TransformComponent*)Entity_GetFixedComponent(entity, COMPONENT_TRANSFORM);
            collider = (ColliderComponent*)Entity_GetFixedComponent(entity, COMPONENT_COLLIDER);
            if (transform == NULL || collider == NULL)
            {
                continue;
            }

            if (collider->shape == COLLIDER_SHAPE_CIRCLE)
            {
                float dx = world_x - transform->x;
                float dy = world_y - transform->y;
                if ((dx * dx) + (dy * dy) <= collider->radius * collider->radius)
                {
                    free(query_entities);
                    return entity;
                }
            }
            else
            {
                float half_w = collider->width * 0.5f;
                float half_h = collider->height * 0.5f;
                if (world_x >= transform->x - half_w && world_x <= transform->x + half_w && world_y >= transform->y - half_h &&
                    world_y <= transform->y + half_h)
                {
                    free(query_entities);
                    return entity;
                }
            }
        }
    }

    free(query_entities);
    return NULL;
}

size_t Scene_QueryEntitiesInAabb(Scene* scene, Aabb bounds, struct Entity** results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_QueryAabb(&scene->spatial_grid, bounds, results, capacity);
}

size_t Scene_QueryEntitiesInRadius(Scene* scene, Vec2 center, float radius, struct Entity** results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_QueryCircle(&scene->spatial_grid, center, radius, results, capacity);
}

size_t Scene_QueryEntitiesAlongSegment(Scene* scene, Vec2 start, Vec2 end, struct Entity** results, size_t capacity)
{
    if (scene == NULL)
    {
        return 0U;
    }

    return SpatialGrid_QuerySegment(&scene->spatial_grid, start, end, results, capacity);
}
