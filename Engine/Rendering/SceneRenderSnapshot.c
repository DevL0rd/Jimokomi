#include "SceneRenderSnapshot.h"

#include "../Scene/Scene.h"
#include "../Scene/Entity.h"
#include "../Scene/Components/ColliderComponent.h"
#include "../Scene/Components/DraggableComponent.h"
#include "../Scene/Components/RenderableComponent.h"
#include "../Scene/Components/RigidBodyComponent.h"
#include "../Scene/Components/SelectableComponent.h"
#include "../Scene/Components/TransformComponent.h"

#include <math.h>
#include <string.h>
#include <time.h>

static double scene_render_snapshot_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
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
    size_t* out_pick_target_count,
    SceneRenderSnapshotProfile* out_profile
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
    if (out_profile != NULL)
    {
        memset(out_profile, 0, sizeof(*out_profile));
    }

    if (desc->profile_culling)
    {
        double started_ms = scene_render_snapshot_now_ms();
        candidate_count = Scene_QueryEntitiesInAabb(
            desc->scene,
            desc->view_bounds,
            desc->scratch_entities,
            desc->scratch_entity_capacity
        );
        if (out_profile != NULL)
        {
            out_profile->cull_grid_ms = scene_render_snapshot_now_ms() - started_ms;
            out_profile->cull_grid_candidates = (uint32_t)candidate_count;
        }
    }
    else
    {
        candidate_count = Scene_QueryEntitiesInAabb(
            desc->scene,
            desc->view_bounds,
            desc->scratch_entities,
            desc->scratch_entity_capacity
        );
        if (out_profile != NULL)
        {
            out_profile->cull_grid_candidates = (uint32_t)candidate_count;
        }
    }

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

void scene_render_snapshot_build(
    const SceneRenderSnapshotDesc* desc,
    RenderSnapshotBuffer* buffer,
    SceneRenderSnapshotProfile* out_profile
)
{
    PhysicsWorldSnapshot empty_physics_snapshot;
    SceneStatsSnapshot scene_stats;
    SpatialGridCellSpan query_span;
    uint64_t snapshot_metadata_signature;
    const PhysicsWorldSnapshot* physics_snapshot;

    if (desc == NULL || desc->scene == NULL || buffer == NULL)
    {
        return;
    }

    memset(&empty_physics_snapshot, 0, sizeof(empty_physics_snapshot));
    physics_snapshot = desc->physics_snapshot != NULL ? desc->physics_snapshot : &empty_physics_snapshot;
    memset(&scene_stats, 0, sizeof(scene_stats));
    Scene_GetStatsSnapshot(desc->scene, &scene_stats);
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
        &buffer->world.pick_target_count,
        out_profile
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
    buffer->stats.overlay.spatial_dirty_cells = scene_stats.spatial_dirty_cell_count;
    buffer->stats.overlay.spatial_dirty_entities = scene_stats.spatial_dirty_entity_count;
    buffer->stats.overlay.physics_hz = physics_snapshot->physics_hz;
    buffer->stats.overlay.physics_ms = physics_snapshot->box2d_step_wall_ms;
    buffer->stats.overlay.physics_pairs_ms = physics_snapshot->profile_pairs_ms;
    buffer->stats.overlay.physics_collide_ms = physics_snapshot->profile_collide_ms;
    buffer->stats.overlay.physics_solve_ms = physics_snapshot->profile_solve_ms;
    buffer->stats.overlay.awake_body_count = physics_snapshot->body_count;
    buffer->stats.overlay.total_body_count = physics_snapshot->total_body_count;
    buffer->stats.overlay.sleeping_body_count = physics_snapshot->sleeping_body_count;
    buffer->stats.overlay.moved_body_count = physics_snapshot->moved_body_count;
    buffer->stats.overlay.render_alpha = desc->render_alpha;
    buffer->stats.render_alpha = desc->render_alpha;
    buffer->stats.cull_grid_ms = out_profile != NULL ? (float)out_profile->cull_grid_ms : 0.0f;
    buffer->stats.cull_grid_candidates = out_profile != NULL ? out_profile->cull_grid_candidates : 0U;

    memset(&query_span, 0, sizeof(query_span));
    if ((desc->profile_culling || buffer->world.draw_debug_world) &&
        Scene_GetSpatialGridCellSpanForAabb(desc->scene, desc->view_bounds, &query_span))
    {
        buffer->world.debug_grid.span_min_x = query_span.min_cell_x;
        buffer->world.debug_grid.span_max_x = query_span.max_cell_x;
        buffer->world.debug_grid.span_min_y = query_span.min_cell_y;
        buffer->world.debug_grid.span_max_y = query_span.max_cell_y;
        buffer->stats.cull_grid_span_min_x = query_span.min_cell_x;
        buffer->stats.cull_grid_span_max_x = query_span.max_cell_x;
        buffer->stats.cull_grid_span_min_y = query_span.min_cell_y;
        buffer->stats.cull_grid_span_max_y = query_span.max_cell_y;
        buffer->stats.cull_grid_span_cells = query_span.cell_count;
    }

    buffer->stats.physics_substeps = desc->physics_substeps;
    buffer->stats.physics_hz = physics_snapshot->physics_hz;
    buffer->stats.physics_step_substeps = physics_snapshot->physics_step_substeps;
    buffer->stats.physics_active_entities = physics_snapshot->active_entity_count;
    buffer->stats.physics_dirty_entities = physics_snapshot->dirty_entity_count;
    buffer->stats.physics_collider_changed_entities = physics_snapshot->collider_changed_entity_count;
    buffer->stats.physics_body_create_queue = physics_snapshot->body_create_count;
    buffer->stats.physics_body_remove_queue = physics_snapshot->body_remove_count;
    buffer->stats.physics_shape_change_queue = physics_snapshot->shape_change_count;
    {
        SceneView scene_view = Scene_GetViewSnapshot(desc->scene);
        buffer->stats.camera_x = scene_view.x;
        buffer->stats.camera_y = scene_view.y;
    }

    if (buffer->world.draw_debug_world && Scene_GetPhysicsWorld(desc->scene) != NULL &&
        buffer->world.debug_collisions != NULL && buffer->world.debug_collision_capacity > 0U)
    {
        PhysicsContactHit contact_hits[256];
        size_t contact_hit_capacity = buffer->world.debug_collision_capacity;
        size_t contact_hit_count;
        size_t collision_index = 0U;
        size_t event_index = 0U;

        if (contact_hit_capacity > sizeof(contact_hits) / sizeof(contact_hits[0]))
        {
            contact_hit_capacity = sizeof(contact_hits) / sizeof(contact_hits[0]);
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
        buffer->world.debug_collision_count = collision_index;
    }
}
