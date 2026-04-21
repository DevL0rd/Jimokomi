#include "PhysicsWorldLifecycleInternal.h"

typedef struct PhysicsQueryAccumulator
{
    PhysicsWorld* world;
    PhysicsQueryResult* results;
    size_t capacity;
    size_t count;
    b2Vec2 ray_origin;
} PhysicsQueryAccumulator;

static bool PhysicsWorld_OverlapCollector(b2ShapeId shape_id, void* context)
{
    PhysicsQueryAccumulator* accumulator = (PhysicsQueryAccumulator*)context;
    b2BodyId body_id;

    if (accumulator == NULL || accumulator->count >= accumulator->capacity)
    {
        return false;
    }

    body_id = b2Shape_GetBody(shape_id);
    accumulator->results[accumulator->count++] = (PhysicsQueryResult){
        PhysicsWorld_GetEntityForBody(accumulator->world, body_id),
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        0.0f
    };
    return true;
}

static float PhysicsWorld_RayCollector(b2ShapeId shape_id, b2Vec2 point, b2Vec2 normal, float fraction, void* context)
{
    PhysicsQueryAccumulator* accumulator = (PhysicsQueryAccumulator*)context;
    b2BodyId body_id;

    if (accumulator == NULL || accumulator->count >= accumulator->capacity)
    {
        return 0.0f;
    }

    body_id = b2Shape_GetBody(shape_id);
    accumulator->results[accumulator->count++] = (PhysicsQueryResult){
        PhysicsWorld_GetEntityForBody(accumulator->world, body_id),
        { point.x, point.y },
        { normal.x, normal.y },
        fraction
    };
    return 1.0f;
}

size_t PhysicsWorld_QueryPoint(PhysicsWorld* world, float x, float y, PhysicsQueryResult* results, size_t capacity)
{
    b2AABB aabb;
    b2QueryFilter filter = b2DefaultQueryFilter();
    PhysicsQueryAccumulator accumulator = {world, results, capacity, 0u, {x, y}};

    if (world == NULL || !world->lifecycle->has_world || results == NULL || capacity == 0)
    {
        return 0;
    }

    aabb.lowerBound = (b2Vec2){x - 0.001f, y - 0.001f};
    aabb.upperBound = (b2Vec2){x + 0.001f, y + 0.001f};
    b2World_OverlapAABB(world->lifecycle->world_id, aabb, filter, PhysicsWorld_OverlapCollector, &accumulator);
    return accumulator.count;
}

size_t PhysicsWorld_QueryRay(PhysicsWorld* world,
                               float x0,
                               float y0,
                               float x1,
                               float y1,
                               PhysicsQueryResult* results,
                               size_t capacity)
{
    b2QueryFilter filter = b2DefaultQueryFilter();
    PhysicsQueryAccumulator accumulator = {world, results, capacity, 0u, {x0, y0}};

    if (world == NULL || !world->lifecycle->has_world || results == NULL || capacity == 0)
    {
        return 0;
    }

    b2World_CastRay(world->lifecycle->world_id,
                    (b2Vec2){x0, y0},
                    (b2Vec2){x1 - x0, y1 - y0},
                    filter,
                    PhysicsWorld_RayCollector,
                    &accumulator);
    return accumulator.count;
}

size_t PhysicsWorld_QueryRadius(PhysicsWorld* world, float x, float y, float radius, PhysicsQueryResult* results, size_t capacity)
{
    b2AABB aabb;
    b2QueryFilter filter = b2DefaultQueryFilter();
    PhysicsQueryAccumulator accumulator = {world, results, capacity, 0u, {x, y}};

    if (world == NULL || !world->lifecycle->has_world || results == NULL || capacity == 0)
    {
        return 0;
    }

    aabb.lowerBound = (b2Vec2){x - radius, y - radius};
    aabb.upperBound = (b2Vec2){x + radius, y + radius};
    b2World_OverlapAABB(world->lifecycle->world_id, aabb, filter, PhysicsWorld_OverlapCollector, &accumulator);
    return accumulator.count;
}

size_t PhysicsWorld_GetContactHitCount(PhysicsWorld* world)
{
    b2ContactEvents contact_events;

    if (world == NULL || !world->lifecycle->has_world) {
        return 0U;
    }

    contact_events = b2World_GetContactEvents(world->lifecycle->world_id);
    return contact_events.hitCount > 0 ? (size_t)contact_events.hitCount : 0U;
}

size_t PhysicsWorld_GetContactHits(PhysicsWorld* world, PhysicsContactHit* hits, size_t capacity)
{
    b2ContactEvents contact_events;
    size_t count = 0U;
    int event_index;

    if (world == NULL || !world->lifecycle->has_world || hits == NULL || capacity == 0U) {
        return 0U;
    }

    contact_events = b2World_GetContactEvents(world->lifecycle->world_id);
    for (event_index = 0; event_index < contact_events.hitCount && count < capacity; ++event_index) {
        const b2ContactHitEvent* hit = &contact_events.hitEvents[event_index];
        PhysicsContactHit* out_hit = &hits[count++];
        b2BodyId body_a = b2Shape_GetBody(hit->shapeIdA);
        b2BodyId body_b = b2Shape_GetBody(hit->shapeIdB);

        out_hit->point.x = hit->point.x;
        out_hit->point.y = hit->point.y;
        out_hit->normal.x = hit->normal.x;
        out_hit->normal.y = hit->normal.y;
        out_hit->entity_a = PhysicsWorld_GetEntityForBody(world, body_a);
        out_hit->entity_b = PhysicsWorld_GetEntityForBody(world, body_b);
    }

    return count;
}
