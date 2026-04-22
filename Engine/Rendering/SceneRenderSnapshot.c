#include "SceneRenderSnapshot.h"
#include "RenderSnapshotInternal.h"

#include "../Physics/PhysicsBodyControl.h"
#include "../Scene/Scene.h"
#include "../Scene/ScenePhysics.h"
#include "../Scene/SceneQueries.h"
#include "../Scene/SceneStats.h"
#include "../Scene/SceneView.h"
#include "../Scene/Entity.h"
#include "../Scene/Components/ColliderComponent.h"
#include "../Scene/Components/DraggableComponent.h"
#include "../Scene/Components/RenderableComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Components/SelectableComponent.h"
#include "../Scene/Components/TransformComponent.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct SceneRenderSnapshotDesc
{
    struct Scene* scene;
    struct Entity** scratch_entities;
    size_t scratch_entity_capacity;
    Aabb view_bounds;
    float render_alpha;
    uint64_t now_ms;
    double update_ms;
    uint32_t physics_substeps;
    bool debug_overlay_enabled;
    bool draw_debug_world;
    uint64_t selected_entity_id;
    uint64_t hovered_entity_id;
    RendererBackdropDrawFn backdrop_draw;
    void* backdrop_user_data;
    uint64_t backdrop_signature;
    const PhysicsWorldSnapshot* physics_snapshot;
} SceneRenderSnapshotDesc;

struct SceneRenderSnapshotBuilder {
    Entity** scratch_entities;
    size_t scratch_entity_capacity;
};

static bool scene_render_snapshot_builder_reserve_entities(
    SceneRenderSnapshotBuilder* builder,
    size_t required_capacity
) {
    Entity** next_entities;
    size_t next_capacity;

    if (builder == NULL) {
        return false;
    }
    if (builder->scratch_entity_capacity >= required_capacity) {
        return true;
    }

    next_capacity = builder->scratch_entity_capacity > 0U
        ? builder->scratch_entity_capacity
        : 64U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_entities = (Entity**)realloc(
        builder->scratch_entities,
        next_capacity * sizeof(*next_entities)
    );
    if (next_entities == NULL) {
        return false;
    }

    builder->scratch_entities = next_entities;
    builder->scratch_entity_capacity = next_capacity;
    return true;
}

SceneRenderSnapshotBuilder* scene_render_snapshot_builder_create(void) {
    return (SceneRenderSnapshotBuilder*)calloc(1U, sizeof(SceneRenderSnapshotBuilder));
}

void scene_render_snapshot_builder_destroy(SceneRenderSnapshotBuilder* builder) {
    if (builder == NULL) {
        return;
    }

    free(builder->scratch_entities);
    free(builder);
}

static float scene_render_snapshot_lerp(float a, float b, float alpha)
{
    return a + ((b - a) * alpha);
}

static float scene_render_snapshot_lerp_angle(float a, float b, float alpha)
{
    float delta = fmodf(b - a, 6.28318530718f);
    if (delta > 3.14159265359f)
    {
        delta -= 6.28318530718f;
    }
    else if (delta < -3.14159265359f)
    {
        delta += 6.28318530718f;
    }
    return a + delta * alpha;
}

static uint64_t scene_render_snapshot_hash_u64(uint64_t hash, uint64_t value)
{
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    return hash;
}

Aabb scene_render_snapshot_compute_view_bounds(
    const struct Scene* scene,
    const Camera* camera,
    float preload_screens
)
{
    Aabb bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    SceneView scene_view;

    if (scene == NULL)
    {
        return bounds;
    }

    scene_view = Scene_GetViewSnapshot(scene);
    bounds.min_x = scene_view.has_world_bounds ? scene_view.world_min_x : 0.0f;
    bounds.min_y = scene_view.has_world_bounds ? scene_view.world_min_y : 0.0f;
    bounds.max_x = scene_view.has_world_bounds ? scene_view.world_max_x : scene_view.view_width;
    bounds.max_y = scene_view.has_world_bounds ? scene_view.world_max_y : scene_view.view_height;
    if (camera != NULL && camera->view_width > 0.0f && camera->view_height > 0.0f)
    {
        float preload_x = camera->view_width * preload_screens;
        float preload_y = camera->view_height * preload_screens;

        bounds.min_x = camera->x - preload_x;
        bounds.min_y = camera->y - preload_y;
        bounds.max_x = camera->x + camera->view_width + preload_x;
        bounds.max_y = camera->y + camera->view_height + preload_y;
        if (scene_view.has_world_bounds)
        {
            bounds.min_x = clamp_f(bounds.min_x, scene_view.world_min_x, scene_view.world_max_x);
            bounds.min_y = clamp_f(bounds.min_y, scene_view.world_min_y, scene_view.world_max_y);
            bounds.max_x = clamp_f(bounds.max_x, scene_view.world_min_x, scene_view.world_max_x);
            bounds.max_y = clamp_f(bounds.max_y, scene_view.world_min_y, scene_view.world_max_y);
        }
    }

    return bounds;
}

static size_t scene_render_snapshot_collect_frame_data(
    const SceneRenderSnapshotDesc* desc,
    SpriteRenderable* items,
    DebugEntityView* entities,
    size_t item_capacity,
    uint64_t* out_item_frame_signature,
    uint64_t* out_item_sort_signature,
    uint64_t* out_item_instance_signature,
    bool* out_items_sorted_by_layer,
    DebugEntityView* out_selected_entity,
    bool* out_has_selected_entity,
    DebugEntityView* out_hovered_entity,
    bool* out_has_hovered_entity,
    PickTargetView* pick_targets,
    size_t pick_target_capacity,
    size_t* out_pick_target_count
)
{
    size_t index;
    size_t visible_count = 0U;
    size_t candidate_count;
    size_t pick_target_count = 0U;
    uint64_t item_frame_signature = 1469598103934665603ULL;
    uint64_t item_sort_signature = 1099511628211ULL;
    uint64_t item_instance_signature = 88172645463325252ULL;
    bool collect_debug;

    if (desc == NULL || desc->scene == NULL || items == NULL || desc->scratch_entities == NULL)
    {
        return 0U;
    }

    if (out_item_frame_signature != NULL) *out_item_frame_signature = item_frame_signature;
    if (out_item_sort_signature != NULL) *out_item_sort_signature = item_sort_signature;
    if (out_item_instance_signature != NULL) *out_item_instance_signature = item_instance_signature;
    if (out_items_sorted_by_layer != NULL) *out_items_sorted_by_layer = true;
    if (out_has_selected_entity != NULL) *out_has_selected_entity = false;
    if (out_has_hovered_entity != NULL) *out_has_hovered_entity = false;
    if (out_pick_target_count != NULL) *out_pick_target_count = 0U;

    candidate_count = Scene_QueryEntitiesInAabb(
        desc->scene,
        desc->view_bounds,
        desc->scratch_entities,
        desc->scratch_entity_capacity
    );

    collect_debug = entities != NULL && desc->debug_overlay_enabled && desc->draw_debug_world;
    for (index = 0U; index < candidate_count && visible_count < item_capacity; ++index)
    {
        Entity* entity = desc->scratch_entities[index];
        TransformComponent* transform;
        RenderableComponent* renderable;
        ColliderComponent* collider;
        SpriteRenderable* item;
        uint64_t entity_id;

        if (entity == NULL || !Entity_IsActive(entity))
        {
            continue;
        }

        transform = (TransformComponent*)Entity_GetComponent(entity, COMPONENT_TRANSFORM);
        renderable = (RenderableComponent*)Entity_GetComponent(entity, COMPONENT_RENDERABLE);
        if (transform == NULL || renderable == NULL || !renderable->visible)
        {
            continue;
        }

        item = &items[visible_count];
        item->x = scene_render_snapshot_lerp(transform->previous_x, transform->x, desc->render_alpha);
        item->y = scene_render_snapshot_lerp(transform->previous_y, transform->y, desc->render_alpha);
        item->angle_radians = scene_render_snapshot_lerp_angle(
            transform->previous_angle_radians,
            transform->angle_radians,
            desc->render_alpha
        );
        item->anchor_x = renderable->anchor_x;
        item->anchor_y = renderable->anchor_y;
        item->layer = renderable->layer;
        item->visible = renderable->visible;
        item->visual_source_handle = renderable->visual_source_handle;
        item->material_handle = renderable->material_handle;
        item->shader_handle = renderable->shader_handle;
        item->user_data = NULL;

        entity_id = (uint64_t)Entity_GetId(entity);
        item_frame_signature = scene_render_snapshot_hash_u64(item_frame_signature, (uint64_t)(int64_t)item->layer);
        item_frame_signature = scene_render_snapshot_hash_u64(item_frame_signature, item->visible ? 1U : 0U);
        item_frame_signature = scene_render_snapshot_hash_u64(item_frame_signature, item->visual_source_handle.id);
        item_frame_signature = scene_render_snapshot_hash_u64(item_frame_signature, item->material_handle.id);
        item_frame_signature = scene_render_snapshot_hash_u64(item_frame_signature, item->shader_handle.id);
        item_instance_signature = scene_render_snapshot_hash_u64(item_instance_signature, item->visual_source_handle.id);
        item_instance_signature = scene_render_snapshot_hash_u64(item_instance_signature, item->material_handle.id);
        item_instance_signature = scene_render_snapshot_hash_u64(item_instance_signature, item->shader_handle.id);

        collider = (ColliderComponent*)Entity_GetComponent(entity, COMPONENT_COLLIDER);
        if (pick_targets != NULL && pick_target_count < pick_target_capacity && collider != NULL)
        {
            DraggableComponent* draggable = (DraggableComponent*)Entity_GetComponent(entity, COMPONENT_DRAGGABLE);
            if (draggable != NULL && draggable->enabled)
            {
                PickTargetView* target = &pick_targets[pick_target_count++];

                target->id = entity_id;
                target->x = item->x;
                target->y = item->y;
                target->radius = draggable->pick_radius;
                target->extent_x = collider->shape == COLLIDER_SHAPE_CIRCLE
                    ? draggable->pick_radius
                    : collider->width * 0.5f;
                target->extent_y = collider->shape == COLLIDER_SHAPE_CIRCLE
                    ? draggable->pick_radius
                    : collider->height * 0.5f;
                target->is_circle = collider->shape == COLLIDER_SHAPE_CIRCLE;
            }
        }

        if (collider != NULL &&
            (collect_debug || entity_id == desc->selected_entity_id || entity_id == desc->hovered_entity_id))
        {
            DebugEntityView debug_entity = { 0 };
            RigidBodyComponent* rigid_body = (RigidBodyComponent*)Entity_GetComponent(entity, COMPONENT_RIGID_BODY);
            SelectableComponent* selectable = (SelectableComponent*)Entity_GetComponent(entity, COMPONENT_SELECTABLE);

            debug_entity.id = entity_id;
            debug_entity.position.x = item->x;
            debug_entity.position.y = item->y;
            debug_entity.angle_radians = item->angle_radians;
            debug_entity.extent_x = collider->shape == COLLIDER_SHAPE_CIRCLE ? collider->radius : collider->width * 0.5f;
            debug_entity.extent_y = collider->shape == COLLIDER_SHAPE_CIRCLE ? collider->radius : collider->height * 0.5f;
            debug_entity.radius = collider->radius;
            debug_entity.is_circle = collider->shape == COLLIDER_SHAPE_CIRCLE;
            debug_entity.is_selected = selectable != NULL
                ? selectable->selected
                : entity_id == desc->selected_entity_id;
            if (rigid_body != NULL && rigid_body->has_body && Scene_GetPhysicsWorld(desc->scene) != NULL)
            {
                Vec2 velocity = { 0.0f, 0.0f };
                bool awake = false;
                PhysicsWorld_GetEntityLinearVelocity(Scene_GetPhysicsWorld(desc->scene), entity, &velocity);
                PhysicsWorld_IsEntityAwake(Scene_GetPhysicsWorld(desc->scene), entity, &awake);
                debug_entity.velocity = velocity;
                debug_entity.is_sleeping = !awake;
                debug_entity.is_moving = fabsf(velocity.x) + fabsf(velocity.y) > 0.05f;
            }
            if (collect_debug)
            {
                entities[visible_count] = debug_entity;
            }
            if (out_selected_entity != NULL && out_has_selected_entity != NULL &&
                !*out_has_selected_entity && entity_id == desc->selected_entity_id)
            {
                *out_selected_entity = debug_entity;
                *out_has_selected_entity = true;
            }
            if (out_hovered_entity != NULL && out_has_hovered_entity != NULL &&
                !*out_has_hovered_entity && entity_id == desc->hovered_entity_id)
            {
                *out_hovered_entity = debug_entity;
                *out_has_hovered_entity = true;
            }
        }

        visible_count += 1U;
    }

    if (out_item_frame_signature != NULL) *out_item_frame_signature = item_frame_signature;
    if (out_item_sort_signature != NULL) *out_item_sort_signature = scene_render_snapshot_hash_u64(item_sort_signature, visible_count);
    if (out_item_instance_signature != NULL) *out_item_instance_signature = item_instance_signature;
    if (out_items_sorted_by_layer != NULL) *out_items_sorted_by_layer = true;
    if (out_pick_target_count != NULL) *out_pick_target_count = pick_target_count;
    return visible_count;
}

static bool scene_render_snapshot_build_desc(
    const SceneRenderSnapshotDesc* desc,
    RenderSnapshotBuffer* buffer
)
{
    PhysicsWorldSnapshot empty_physics_snapshot;
    SceneStatsSnapshot scene_stats;
    SpatialGridCellSpan query_span;
    uint64_t snapshot_metadata_signature;
    const PhysicsWorldSnapshot* physics_snapshot;
    size_t entity_capacity;
    size_t contact_hit_capacity;

    if (desc == NULL || desc->scene == NULL || buffer == NULL)
    {
        return false;
    }

    memset(&empty_physics_snapshot, 0, sizeof(empty_physics_snapshot));
    physics_snapshot = desc->physics_snapshot != NULL ? desc->physics_snapshot : &empty_physics_snapshot;
    memset(&scene_stats, 0, sizeof(scene_stats));
    Scene_GetStatsSnapshot(desc->scene, &scene_stats);
    entity_capacity = Scene_GetEntityCount(desc->scene);
    if (!render_world_snapshot_reserve_items(&buffer->world, entity_capacity))
    {
        return false;
    }
    contact_hit_capacity = Scene_GetPhysicsWorld(desc->scene) != NULL
        ? PhysicsWorld_GetContactHitCount(Scene_GetPhysicsWorld(desc->scene))
        : 0U;
    if (!render_world_snapshot_reserve_debug_collisions(&buffer->world, contact_hit_capacity))
    {
        return false;
    }
    render_world_snapshot_reset(&buffer->world);
    memset(&buffer->stats, 0, sizeof(buffer->stats));

    buffer->world.backdrop_draw = desc->backdrop_draw;
    buffer->world.backdrop_user_data = desc->backdrop_user_data;
    buffer->world.backdrop_signature = desc->backdrop_signature;
    buffer->world.draw_sprites = true;
    buffer->world.draw_debug_world = desc->debug_overlay_enabled && desc->draw_debug_world;
    buffer->world.now_ms = desc->now_ms;
    buffer->world.item_count = scene_render_snapshot_collect_frame_data(
        desc,
        buffer->world.items,
        buffer->world.debug_entities,
        buffer->world.item_capacity,
        &buffer->world.item_frame_signature,
        &buffer->world.item_sort_signature,
        &buffer->world.item_instance_signature,
        &buffer->world.items_sorted_by_layer,
        &buffer->world.selected_entity,
        &buffer->world.has_selected_entity,
        &buffer->world.hovered_entity,
        &buffer->world.has_hovered_entity,
        buffer->world.pick_targets,
        buffer->world.pick_target_capacity,
        &buffer->world.pick_target_count
    );
    buffer->world.item_signatures_valid = true;
    buffer->world.debug_entity_count = buffer->world.draw_debug_world ? buffer->world.item_count : 0U;

    {
        SpatialGridStatsSnapshot spatial_stats;
        Scene_GetSpatialGridStatsSnapshot(desc->scene, &spatial_stats);
        if (spatial_stats.enabled)
        {
            buffer->world.debug_grid.enabled = true;
            buffer->world.debug_grid.min_x = spatial_stats.bounds.min_x;
            buffer->world.debug_grid.min_y = spatial_stats.bounds.min_y;
            buffer->world.debug_grid.max_x = spatial_stats.bounds.max_x;
            buffer->world.debug_grid.max_y = spatial_stats.bounds.max_y;
            buffer->world.debug_grid.cell_size = spatial_stats.cell_size;
        }
    }

    snapshot_metadata_signature = 1469598103934665603ULL;
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.item_count);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.debug_entity_count);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.draw_sprites ? 1U : 0U);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.draw_debug_world ? 1U : 0U);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, buffer->world.backdrop_signature);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, desc->selected_entity_id);
    snapshot_metadata_signature = scene_render_snapshot_hash_u64(snapshot_metadata_signature, desc->hovered_entity_id);
    buffer->world.metadata_signature = snapshot_metadata_signature;
    buffer->world.metadata_dirty = true;

    buffer->stats.overlay.update_ms = (float)desc->update_ms;
    buffer->stats.overlay.sim_ms = (float)desc->update_ms;
    buffer->stats.overlay.physics_substeps = desc->physics_substeps;
    buffer->stats.overlay.visible_count = (uint32_t)Scene_GetEntityCount(desc->scene);
    buffer->stats.overlay.physics_hz = physics_snapshot->physics_hz;
    buffer->stats.overlay.physics_ms = physics_snapshot->corephys_step_wall_ms;
    buffer->stats.overlay.awake_body_count = physics_snapshot->body_count;
    buffer->stats.overlay.total_body_count = physics_snapshot->total_body_count;
    buffer->stats.overlay.sleeping_body_count = physics_snapshot->sleeping_body_count;
    buffer->stats.overlay.moved_body_count = physics_snapshot->moved_body_count;
    buffer->stats.overlay.physics_task_worker_count = physics_snapshot->task_worker_count;
    buffer->stats.overlay.physics_task_background_thread_count = physics_snapshot->task_background_thread_count;
    buffer->stats.overlay.physics_task_enqueued_count = physics_snapshot->corephys_enqueued_task_count;
    buffer->stats.overlay.physics_task_inline_count = physics_snapshot->corephys_inline_task_count;
    buffer->stats.overlay.physics_task_main_chunk_count = physics_snapshot->corephys_main_chunk_count;
    buffer->stats.overlay.physics_task_worker_chunk_count = physics_snapshot->corephys_worker_chunk_count;
    buffer->stats.overlay.render_alpha = desc->render_alpha;
    buffer->stats.render.render_alpha = desc->render_alpha;

    memset(&query_span, 0, sizeof(query_span));
    if (buffer->world.draw_debug_world &&
        Scene_GetSpatialGridCellSpanForAabb(desc->scene, desc->view_bounds, &query_span))
    {
        buffer->world.debug_grid.span_min_x = query_span.min_cell_x;
        buffer->world.debug_grid.span_max_x = query_span.max_cell_x;
        buffer->world.debug_grid.span_min_y = query_span.min_cell_y;
        buffer->world.debug_grid.span_max_y = query_span.max_cell_y;
    }

    buffer->stats.physics.physics_substeps = desc->physics_substeps;
    buffer->stats.physics.physics_hz = physics_snapshot->physics_hz;
    buffer->stats.physics.physics_step_substeps = physics_snapshot->physics_step_substeps;
    buffer->stats.physics.physics_active_entities = physics_snapshot->active_entity_count;
    buffer->stats.physics.physics_dirty_entities = physics_snapshot->dirty_entity_count;
    buffer->stats.physics.physics_collider_changed_entities = physics_snapshot->collider_changed_entity_count;
    buffer->stats.physics.physics_body_create_queue = physics_snapshot->body_create_count;
    buffer->stats.physics.physics_body_remove_queue = physics_snapshot->body_remove_count;
    buffer->stats.physics.physics_shape_change_queue = physics_snapshot->shape_change_count;
    buffer->stats.physics.physics_task_worker_count = physics_snapshot->task_worker_count;
    buffer->stats.physics.physics_task_background_thread_count = physics_snapshot->task_background_thread_count;
    buffer->stats.physics.physics_task_enqueued_count = physics_snapshot->corephys_enqueued_task_count;
    buffer->stats.physics.physics_task_inline_count = physics_snapshot->corephys_inline_task_count;
    buffer->stats.physics.physics_task_main_chunk_count = physics_snapshot->corephys_main_chunk_count;
    buffer->stats.physics.physics_task_worker_chunk_count = physics_snapshot->corephys_worker_chunk_count;
    {
        SceneView scene_view = Scene_GetViewSnapshot(desc->scene);
        buffer->stats.camera.camera_x = scene_view.x;
        buffer->stats.camera.camera_y = scene_view.y;
    }

    if (contact_hit_capacity > 0U &&
        buffer->world.draw_debug_world && Scene_GetPhysicsWorld(desc->scene) != NULL &&
        buffer->world.debug_collisions != NULL && buffer->world.debug_collision_capacity > 0U)
    {
        PhysicsContactHit* contact_hits;
        size_t contact_hit_count;
        size_t collision_index = 0U;
        size_t event_index = 0U;

        contact_hits = (PhysicsContactHit*)calloc(contact_hit_capacity, sizeof(*contact_hits));
        if (contact_hits == NULL)
        {
            return false;
        }
        contact_hit_count = PhysicsWorld_GetContactHits(Scene_GetPhysicsWorld(desc->scene), contact_hits, contact_hit_capacity);
        for (event_index = 0U; event_index < contact_hit_count &&
                               collision_index < buffer->world.debug_collision_capacity; ++event_index)
        {
            const PhysicsContactHit* hit = &contact_hits[event_index];
            DebugCollisionView* collision = &buffer->world.debug_collisions[collision_index++];

            collision->point_a = hit->point;
            collision->point_b = hit->point;
            collision->contact_point = hit->point;
            collision->contact_normal = hit->normal;
            collision->active = true;
            collision->sensor = false;
            collision->sleeping = false;
        }
        free(contact_hits);
        buffer->world.debug_collision_count = collision_index;
    }

    return true;
}

bool scene_render_snapshot_build(
    SceneRenderSnapshotBuilder* builder,
    struct Scene* scene,
    Aabb view_bounds,
    float render_alpha,
    uint64_t now_ms,
    double update_ms,
    uint32_t physics_substeps,
    bool debug_overlay_enabled,
    bool draw_debug_world,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id,
    RendererBackdropDrawFn backdrop_draw,
    void* backdrop_user_data,
    uint64_t backdrop_signature,
    RenderSnapshotBuffer* buffer
) {
    PhysicsWorldSnapshot physics_snapshot;
    SceneRenderSnapshotDesc desc;

    if (builder == NULL || scene == NULL || buffer == NULL) {
        return false;
    }
    if (!scene_render_snapshot_builder_reserve_entities(builder, Scene_GetEntityCount(scene))) {
        return false;
    }

    memset(&physics_snapshot, 0, sizeof(physics_snapshot));
    if (Scene_GetPhysicsWorld(scene) != NULL) {
        PhysicsWorld_GetSnapshot(Scene_GetPhysicsWorld(scene), &physics_snapshot);
    }

    memset(&desc, 0, sizeof(desc));
    desc.scene = scene;
    desc.scratch_entities = builder->scratch_entities;
    desc.scratch_entity_capacity = builder->scratch_entity_capacity;
    desc.view_bounds = view_bounds;
    desc.render_alpha = render_alpha;
    desc.now_ms = now_ms;
    desc.update_ms = update_ms;
    desc.physics_substeps = physics_substeps;
    desc.debug_overlay_enabled = debug_overlay_enabled;
    desc.draw_debug_world = draw_debug_world;
    desc.selected_entity_id = selected_entity_id;
    desc.hovered_entity_id = hovered_entity_id;
    desc.backdrop_draw = backdrop_draw;
    desc.backdrop_user_data = backdrop_user_data;
    desc.backdrop_signature = backdrop_signature;
    desc.physics_snapshot = &physics_snapshot;

    return scene_render_snapshot_build_desc(&desc, buffer);
}
