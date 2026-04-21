#include "Game/GameInternal.h"

#include <math.h>
#include <string.h>

Aabb game_compute_snapshot_view_bounds(
    const GameState* game,
    const GameInputPacket* input_packet,
    bool has_input
) {
    Aabb bounds = { 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT };

    if (game == NULL || input_packet == NULL || !has_input ||
        input_packet->camera_view_width <= 0.0f ||
        input_packet->camera_view_height <= 0.0f) {
        return bounds;
    }

    {
        float preload_x = input_packet->camera_view_width * SPATIAL_QUERY_PRELOAD_SCREENS;
        float preload_y = input_packet->camera_view_height * SPATIAL_QUERY_PRELOAD_SCREENS;

        bounds.min_x = input_packet->camera_x - preload_x;
        bounds.min_y = input_packet->camera_y - preload_y;
        bounds.max_x = input_packet->camera_x + input_packet->camera_view_width + preload_x;
        bounds.max_y = input_packet->camera_y + input_packet->camera_view_height + preload_y;
    }

    if (game->scene != NULL) {
        SceneView scene_view = Scene_GetViewSnapshot(game->scene);
        if (scene_view.has_world_bounds) {
            bounds.min_x = clamp_f(bounds.min_x, scene_view.world_min_x, scene_view.world_max_x);
            bounds.min_y = clamp_f(bounds.min_y, scene_view.world_min_y, scene_view.world_max_y);
            bounds.max_x = clamp_f(bounds.max_x, scene_view.world_min_x, scene_view.world_max_x);
            bounds.max_y = clamp_f(bounds.max_y, scene_view.world_min_y, scene_view.world_max_y);
        }
    }

    return bounds;
}

Aabb game_get_cached_snapshot_view_bounds(
    GameState* game,
    const GameInputPacket* input_packet,
    bool has_input
) {
    bool input_has_camera =
        input_packet != NULL &&
        has_input &&
        input_packet->camera_view_width > 0.0f &&
        input_packet->camera_view_height > 0.0f;

    if (game == NULL) {
        return (Aabb){ 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT };
    }

    if (game->cached_snapshot_view_bounds_valid &&
        game->cached_snapshot_has_input == input_has_camera &&
        (!input_has_camera ||
         (input_packet->camera_x == game->cached_snapshot_camera_x &&
          input_packet->camera_y == game->cached_snapshot_camera_y &&
          input_packet->camera_view_width == game->cached_snapshot_camera_view_width &&
          input_packet->camera_view_height == game->cached_snapshot_camera_view_height))) {
        return game->cached_snapshot_view_bounds;
    }

    game->cached_snapshot_view_bounds =
        game_compute_snapshot_view_bounds(game, input_packet, input_has_camera);
    game->cached_snapshot_has_input = input_has_camera;
    if (input_has_camera) {
        game->cached_snapshot_camera_x = input_packet->camera_x;
        game->cached_snapshot_camera_y = input_packet->camera_y;
        game->cached_snapshot_camera_view_width = input_packet->camera_view_width;
        game->cached_snapshot_camera_view_height = input_packet->camera_view_height;
    } else {
        game->cached_snapshot_camera_x = 0.0f;
        game->cached_snapshot_camera_y = 0.0f;
        game->cached_snapshot_camera_view_width = 0.0f;
        game->cached_snapshot_camera_view_height = 0.0f;
    }
    game->cached_snapshot_view_bounds_valid = true;
    return game->cached_snapshot_view_bounds;
}

size_t game_collect_frame_data(
    GameState *game,
    SpriteRenderable *items,
    DebugEntityView *entities,
    size_t item_capacity,
    float render_alpha,
    bool capture_all_debug_details,
    size_t selected_ball_index,
    size_t hovered_ball_index,
    Aabb view_bounds,
    GameCullProfile* cull_profile,
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
) {
    size_t index;
    size_t visible_count = 0U;
    size_t candidate_count = 0U;
    bool collect_debug = false;
    bool needs_entity_debug = false;
    size_t pick_target_count = 0U;
    uint64_t selected_entity_id = selected_ball_index == INVALID_INDEX ? 0U : (uint64_t)(selected_ball_index + 1U);
    uint64_t hovered_entity_id = hovered_ball_index == INVALID_INDEX ? 0U : (uint64_t)(hovered_ball_index + 1U);
    uint64_t item_frame_signature = 1469598103934665603ULL;
    uint64_t item_sort_signature = 1099511628211ULL;
    uint64_t item_instance_signature = 88172645463325252ULL;
    if (game == NULL || items == NULL) {
        return 0U;
    }

    if (out_item_frame_signature != NULL) {
        *out_item_frame_signature = item_frame_signature;
    }
    if (out_item_sort_signature != NULL) {
        *out_item_sort_signature = item_sort_signature;
    }
    if (out_item_instance_signature != NULL) {
        *out_item_instance_signature = item_instance_signature;
    }
    if (out_items_sorted_by_layer != NULL) {
        *out_items_sorted_by_layer = true;
    }
    if (out_has_selected_entity != NULL) {
        *out_has_selected_entity = false;
    }
    if (out_has_hovered_entity != NULL) {
        *out_has_hovered_entity = false;
    }
    if (out_pick_target_count != NULL) {
        *out_pick_target_count = 0U;
    }

    collect_debug = entities != NULL && capture_all_debug_details;
    needs_entity_debug = collect_debug || selected_entity_id != 0U || hovered_entity_id != 0U;
    if (cull_profile != NULL) {
        memset(cull_profile, 0, sizeof(*cull_profile));
    }
    if (game->engine.profiler.enabled) {
        double started_ms = game_now_ms();
        candidate_count = Scene_QueryEntitiesInAabb(
            game->scene,
            view_bounds,
            game->visible_query_entities,
            item_capacity < BALL_COUNT ? item_capacity : BALL_COUNT
        );
        if (cull_profile != NULL) {
            cull_profile->grid_ms = game_now_ms() - started_ms;
            cull_profile->grid_candidates = (uint32_t)candidate_count;
        }
    } else {
        candidate_count = Scene_QueryEntitiesInAabb(
            game->scene,
            view_bounds,
            game->visible_query_entities,
            item_capacity < BALL_COUNT ? item_capacity : BALL_COUNT
        );
    }

    for (index = 0U; index < candidate_count && visible_count < item_capacity; ++index) {
        Entity* entity = game->visible_query_entities[index];
        Ball* ball = NULL;
        TransformComponent* transform = NULL;
        SpriteRenderable* item;

        if (entity == NULL) {
            continue;
        }

        ball = (Ball*)Entity_GetUserData(entity);
        if (ball == NULL || ball->entity != entity) {
            continue;
        }

        transform = ball->transform;
        if (transform == NULL) {
            continue;
        }

        item = &items[visible_count];
        item->x = game_lerp(transform->previous_x, transform->x, render_alpha);
        item->y = game_lerp(transform->previous_y, transform->y, render_alpha);
        item->angle_radians = game_lerp_angle(
            transform->previous_angle_radians,
            transform->angle_radians,
            render_alpha
        );
        item->anchor_x = 0.5f;
        item->anchor_y = 0.5f;
        item->layer = 0;
        item->visible = true;
        item->visual_source_handle = ball->visual_source_handle;
        item->material_handle = ball->material_handle;
        item->shader_handle = game->shared_ball_shader_handle;
        item->user_data = NULL;
        if (ball->visual_state_dirty) {
            game_clear_ball_visual_state_dirty(ball);
        }
        if (ball->highlight_state_dirty) {
            game_clear_ball_highlight_dirty(ball);
        }
        item_frame_signature = game_hash_u64(item_frame_signature, (uint64_t)(int64_t)item->layer);
        item_frame_signature = game_hash_u64(item_frame_signature, item->visual_source_handle.id);
        item_frame_signature = game_hash_u64(item_frame_signature, item->material_handle.id);
        item_frame_signature = game_hash_u64(item_frame_signature, item->shader_handle.id);
        item_instance_signature = game_hash_u64(item_instance_signature, item->visual_source_handle.id);
        item_instance_signature = game_hash_u64(item_instance_signature, item->material_handle.id);
        item_instance_signature = game_hash_u64(item_instance_signature, item->shader_handle.id);

        if (pick_targets != NULL &&
            pick_target_count < pick_target_capacity &&
            ball->interaction != NULL &&
            ball->interaction->draggable &&
            ball->collider != NULL) {
            PickTargetView* target = &pick_targets[pick_target_count++];

            target->id = (uint64_t)Entity_GetId(entity);
            target->x = item->x;
            target->y = item->y;
            target->radius = ball->interaction->pick_radius;
            target->extent_x = ball->collider->shape == COLLIDER_SHAPE_CIRCLE
                ? ball->interaction->pick_radius
                : ball->collider->width * 0.5f;
            target->extent_y = ball->collider->shape == COLLIDER_SHAPE_CIRCLE
                ? ball->interaction->pick_radius
                : ball->collider->height * 0.5f;
            target->is_circle = ball->collider->shape == COLLIDER_SHAPE_CIRCLE;
        }

        if (needs_entity_debug &&
            (collect_debug ||
             (selected_entity_id != 0U && (uint64_t)Entity_GetId(entity) == selected_entity_id) ||
             (hovered_entity_id != 0U && (uint64_t)Entity_GetId(entity) == hovered_entity_id))) {
            DebugEntityView debug_entity = { 0 };
            bool need_debug_detail =
                capture_all_debug_details ||
                (selected_entity_id != 0U && (uint64_t)Entity_GetId(entity) == selected_entity_id) ||
                (hovered_entity_id != 0U && (uint64_t)Entity_GetId(entity) == hovered_entity_id);
            if (ball->collider != NULL) {
                debug_entity.id = (uint64_t)Entity_GetId(entity);
                debug_entity.position.x = item->x;
                debug_entity.position.y = item->y;
                debug_entity.angle_radians = item->angle_radians;
                debug_entity.extent_x = ball->collider->shape == COLLIDER_SHAPE_CIRCLE ? ball->collider->radius : ball->collider->width * 0.5f;
                debug_entity.extent_y = ball->collider->shape == COLLIDER_SHAPE_CIRCLE ? ball->collider->radius : ball->collider->height * 0.5f;
                debug_entity.radius = ball->collider->radius;
                debug_entity.is_circle = ball->collider->shape == COLLIDER_SHAPE_CIRCLE;
                debug_entity.is_selected = debug_entity.id == selected_entity_id;
                if (need_debug_detail && ball->rigid_body != NULL && ball->rigid_body->has_body) {
                    Vec2 velocity = { 0.0f, 0.0f };
                    bool is_awake = false;
                    PhysicsWorld_GetEntityLinearVelocity(Scene_GetPhysicsWorld(game->scene), entity, &velocity);
                    PhysicsWorld_IsEntityAwake(Scene_GetPhysicsWorld(game->scene), entity, &is_awake);
                    debug_entity.velocity = velocity;
                    debug_entity.is_sleeping = !is_awake;
                    debug_entity.is_moving = fabsf(velocity.x) + fabsf(velocity.y) > 0.05f;
                }
                if (collect_debug) {
                    entities[visible_count] = debug_entity;
                }
                if (out_selected_entity != NULL &&
                    out_has_selected_entity != NULL &&
                    !*out_has_selected_entity &&
                    debug_entity.id == selected_entity_id) {
                    *out_selected_entity = debug_entity;
                    *out_has_selected_entity = true;
                }
                if (out_hovered_entity != NULL &&
                    out_has_hovered_entity != NULL &&
                    !*out_has_hovered_entity &&
                    debug_entity.id == hovered_entity_id) {
                    *out_hovered_entity = debug_entity;
                    *out_has_hovered_entity = true;
                }
            }
        }

        visible_count += 1U;
    }

    if (out_item_frame_signature != NULL) {
        *out_item_frame_signature = item_frame_signature;
    }
    if (out_item_sort_signature != NULL) {
        *out_item_sort_signature = game_hash_u64(item_sort_signature, visible_count);
    }
    if (out_item_instance_signature != NULL) {
        *out_item_instance_signature = item_instance_signature;
    }
    if (out_items_sorted_by_layer != NULL) {
        *out_items_sorted_by_layer = true;
    }
    if (out_pick_target_count != NULL) {
        *out_pick_target_count = pick_target_count;
    }
    return visible_count;
}

void game_build_render_snapshot(
    GameState* game,
    RenderSnapshotBuffer* buffer,
    float render_alpha,
    uint64_t now_ms,
    double update_ms,
    double fixed_step_wall_ms,
    uint32_t physics_substeps,
    bool debug_overlay_enabled,
    bool draw_debug_world,
    Aabb view_bounds,
    GameCullProfile* cull_profile,
    const PhysicsWorldSnapshot* physics_snapshot
) {
    PhysicsWorldSnapshot empty_physics_snapshot;
    SceneStatsSnapshot scene_stats;
    SpatialGridCellSpan query_span;
    TransformComponent* player_transform;
    RigidBodyComponent* player_body;
    uint64_t snapshot_metadata_signature;
    const PhysicsWorldSnapshot* source_physics_snapshot = physics_snapshot;

    if (game == NULL || buffer == NULL) {
        return;
    }
    if (source_physics_snapshot == NULL) {
        memset(&empty_physics_snapshot, 0, sizeof(empty_physics_snapshot));
        source_physics_snapshot = &empty_physics_snapshot;
    }

    memset(&scene_stats, 0, sizeof(scene_stats));
    if (game->scene != NULL) {
        Scene_GetStatsSnapshot(game->scene, &scene_stats);
    }

    render_world_snapshot_reset(&buffer->world);
    memset(&buffer->stats, 0, sizeof(buffer->stats));

    buffer->world.backdrop_draw = game_draw_world_backdrop;
    buffer->world.backdrop_user_data = &game->backdrop;
    buffer->world.backdrop_signature = game_backdrop_signature(&game->backdrop);
    buffer->world.draw_sprites = true;
    buffer->world.draw_debug_world = debug_overlay_enabled && draw_debug_world;
    buffer->world.now_ms = now_ms;
    buffer->world.item_count = game_collect_frame_data(
        game,
        buffer->world.items,
        buffer->world.debug_entities,
        buffer->world.item_capacity,
        render_alpha,
        debug_overlay_enabled && draw_debug_world,
        game->selected_ball_index,
        game->hovered_ball_index,
        view_bounds,
        cull_profile,
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
    buffer->world.debug_collision_count = 0U;
    if (game->scene != NULL) {
        SpatialGridStatsSnapshot spatial_stats;

        Scene_GetSpatialGridStatsSnapshot(game->scene, &spatial_stats);
        if (spatial_stats.enabled) {
            buffer->world.debug_grid.enabled = true;
            buffer->world.debug_grid.min_x = spatial_stats.bounds.min_x;
            buffer->world.debug_grid.min_y = spatial_stats.bounds.min_y;
            buffer->world.debug_grid.max_x = spatial_stats.bounds.max_x;
            buffer->world.debug_grid.max_y = spatial_stats.bounds.max_y;
            buffer->world.debug_grid.cell_size = spatial_stats.cell_size;
        }
    }

    snapshot_metadata_signature = 1469598103934665603ULL;
    snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, buffer->world.item_count);
    snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, buffer->world.debug_entity_count);
    snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, buffer->world.draw_sprites ? 1U : 0U);
    snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, buffer->world.draw_debug_world ? 1U : 0U);
    snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, buffer->world.backdrop_signature);
    snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, game->selected_ball_index);
    snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, game->hovered_ball_index);
    if (buffer->world.has_selected_entity) {
        snapshot_metadata_signature = game_hash_u64(snapshot_metadata_signature, buffer->world.selected_entity.id);
    }
    buffer->world.metadata_signature = snapshot_metadata_signature;
    buffer->world.metadata_dirty = snapshot_metadata_signature != game->last_snapshot_metadata_signature;
    game->last_snapshot_metadata_signature = snapshot_metadata_signature;
    game_clear_backdrop_dirty(&game->backdrop);

    buffer->stats.overlay.fps = 0.0f;
    buffer->stats.overlay.update_ms = (float)update_ms;
    buffer->stats.overlay.sim_ms = (float)update_ms;
    buffer->stats.overlay.optimizer_ms = (float)fixed_step_wall_ms;
    buffer->stats.overlay.draw_ms = 0.0f;
    buffer->stats.overlay.physics_ms = 0.0f;
    buffer->stats.overlay.snapshot_age_ms = 0.0f;
    buffer->stats.overlay.physics_hz = 0.0f;
    buffer->stats.overlay.render_alpha = render_alpha;
    buffer->stats.overlay.physics_pairs_ms = 0.0f;
    buffer->stats.overlay.physics_collide_ms = 0.0f;
    buffer->stats.overlay.physics_solve_ms = 0.0f;
    buffer->stats.overlay.physics_substeps = physics_substeps;
    buffer->stats.overlay.visible_count = game->scene != NULL ? (uint32_t)Scene_GetEntityCount(game->scene) : 0U;
    buffer->stats.overlay.awake_body_count = 0U;
    buffer->stats.overlay.total_body_count = 0U;
    buffer->stats.overlay.sleeping_body_count = 0U;
    buffer->stats.overlay.moved_body_count = 0U;
    buffer->stats.overlay.spatial_dirty_cells = game->scene != NULL ? scene_stats.spatial_dirty_cell_count : 0U;
    buffer->stats.overlay.spatial_dirty_entities = game->scene != NULL ? scene_stats.spatial_dirty_entity_count : 0U;
    buffer->stats.render_alpha = render_alpha;
    buffer->stats.cull_grid_ms = cull_profile != NULL ? (float)cull_profile->grid_ms : 0.0f;
    buffer->stats.cull_grid_candidates = cull_profile != NULL ? cull_profile->grid_candidates : 0U;
    memset(&query_span, 0, sizeof(query_span));
    if (game->scene != NULL &&
        (game->engine.profiler.enabled || buffer->world.draw_debug_world) &&
        Scene_GetSpatialGridCellSpanForAabb(game->scene, view_bounds, &query_span)) {
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
    buffer->stats.physics_substeps = physics_substeps;
    buffer->stats.pending_bakes = 0U;
    buffer->stats.tracked_ball_index = game->tracked_ball_index;
    buffer->stats.selected_ball_index = INVALID_INDEX;
    buffer->stats.drag_active = false;
    if (game->scene != NULL) {
        SceneView scene_view = Scene_GetViewSnapshot(game->scene);
        buffer->stats.camera_x = scene_view.x;
        buffer->stats.camera_y = scene_view.y;
    }
    if (game->scene != NULL && Scene_GetPhysicsWorld(game->scene) != NULL) {
        if (buffer->world.draw_debug_world &&
            buffer->world.debug_collisions != NULL &&
            buffer->world.debug_collision_capacity > 0U) {
            PhysicsContactHit contact_hits[256];
            size_t collision_index = 0U;
            size_t contact_hit_count;
            size_t event_index = 0U;
            size_t contact_hit_capacity = buffer->world.debug_collision_capacity;
            if (contact_hit_capacity > sizeof(contact_hits) / sizeof(contact_hits[0])) {
                contact_hit_capacity = sizeof(contact_hits) / sizeof(contact_hits[0]);
            }
            contact_hit_count = PhysicsWorld_GetContactHits(Scene_GetPhysicsWorld(game->scene), contact_hits, contact_hit_capacity);
            for (event_index = 0U; event_index < contact_hit_count &&
                                   collision_index < buffer->world.debug_collision_capacity; ++event_index) {
                const PhysicsContactHit* hit = &contact_hits[event_index];
                DebugCollisionView* collision = &buffer->world.debug_collisions[collision_index++];
                collision->point_a = hit->point;
                collision->point_b = collision->point_a;
                collision->contact_point = collision->point_a;
                collision->contact_normal = hit->normal;
                collision->active = true;
                collision->sensor = false;
                collision->sleeping = false;
            }
            buffer->world.debug_collision_count = collision_index;
        }
        buffer->stats.overlay.physics_hz = source_physics_snapshot->physics_hz;
        buffer->stats.overlay.physics_ms = source_physics_snapshot->box2d_step_wall_ms;
        buffer->stats.overlay.physics_pairs_ms = source_physics_snapshot->profile_pairs_ms;
        buffer->stats.overlay.physics_collide_ms = source_physics_snapshot->profile_collide_ms;
        buffer->stats.overlay.physics_solve_ms = source_physics_snapshot->profile_solve_ms;
        buffer->stats.overlay.awake_body_count = source_physics_snapshot->body_count;
        buffer->stats.overlay.total_body_count = source_physics_snapshot->total_body_count;
        buffer->stats.overlay.sleeping_body_count = source_physics_snapshot->sleeping_body_count;
        buffer->stats.overlay.moved_body_count = source_physics_snapshot->moved_body_count;
        buffer->stats.physics_hz = source_physics_snapshot->physics_hz;
        buffer->stats.physics_step_substeps = source_physics_snapshot->physics_step_substeps;
        buffer->stats.physics_tuner_level = source_physics_snapshot->tuner_level_index;
        buffer->stats.physics_active_entities = source_physics_snapshot->active_entity_count;
        buffer->stats.physics_dirty_entities = source_physics_snapshot->dirty_entity_count;
        buffer->stats.physics_collider_changed_entities = source_physics_snapshot->collider_changed_entity_count;
        buffer->stats.physics_body_create_queue = source_physics_snapshot->body_create_count;
        buffer->stats.physics_body_remove_queue = source_physics_snapshot->body_remove_count;
        buffer->stats.physics_shape_change_queue = source_physics_snapshot->shape_change_count;
    }

    player_transform = game->balls[PLAYER_INDEX].transform;
    player_body = game->balls[PLAYER_INDEX].rigid_body;
    buffer->stats.player_x = player_transform != NULL ? player_transform->x : 0.0f;
    buffer->stats.player_y = player_transform != NULL ? player_transform->y : 0.0f;
    if (player_body != NULL && player_body->has_body) {
        Vec2 velocity = { 0.0f, 0.0f };
        PhysicsWorld_GetEntityLinearVelocity(Scene_GetPhysicsWorld(game->scene), game->balls[PLAYER_INDEX].entity, &velocity);
        buffer->stats.player_vx = velocity.x;
        buffer->stats.player_vy = velocity.y;
    }
}
