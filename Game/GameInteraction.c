#include "Game/GameInternal.h"

#include <math.h>

void game_get_camera_viewport_size(
    const Renderer* renderer,
    float* out_width,
    float* out_height
) {
    float width = WORLD_WIDTH;
    float height = WORLD_HEIGHT;
    int renderer_width = 0;
    int renderer_height = 0;

    if (renderer != NULL) {
        renderer_get_viewport_size(renderer, &renderer_width, &renderer_height);
        if (renderer_width > 0) {
            width = (float)renderer_width;
        }
        if (renderer_height > 0) {
            height = (float)renderer_height;
        }
    }
    if (out_width != NULL) {
        *out_width = width;
    }
    if (out_height != NULL) {
        *out_height = height;
    }
}

void game_render_clamp_camera(Camera* camera) {
    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = WORLD_WIDTH;
    float max_y = WORLD_HEIGHT;
    float max_camera_x = 0.0f;
    float max_camera_y = 0.0f;

    if (camera == NULL) {
        return;
    }

    if (camera->world_bounds.w > 0.0f && camera->world_bounds.h > 0.0f) {
        min_x = camera->world_bounds.x;
        min_y = camera->world_bounds.y;
        max_x = camera->world_bounds.x + camera->world_bounds.w;
        max_y = camera->world_bounds.y + camera->world_bounds.h;
    }

    camera->view_width = clamp_f(camera->view_width, CAMERA_MIN_VIEW_WIDTH, max_x - min_x);
    camera->view_height = clamp_f(camera->view_height, CAMERA_MIN_VIEW_HEIGHT, max_y - min_y);
    max_camera_x = max_x - camera->view_width;
    max_camera_y = max_y - camera->view_height;
    if (max_camera_x < min_x) {
        max_camera_x = min_x;
    }
    if (max_camera_y < min_y) {
        max_camera_y = min_y;
    }
    camera->x = clamp_f(camera->x, min_x, max_camera_x);
    camera->y = clamp_f(camera->y, min_y, max_camera_y);
}

void game_apply_render_camera_zoom(
    Renderer* renderer,
    float zoom_steps,
    Vec2 anchor_screen
) {
    Camera* camera;
    float viewport_width = WORLD_WIDTH;
    float viewport_height = WORLD_HEIGHT;
    float zoom_factor;
    float old_view_width;
    float old_view_height;
    float new_view_width;
    float new_view_height;
    float anchor_u;
    float anchor_v;
    Vec2 anchor_world;

    if (renderer == NULL || fabsf(zoom_steps) <= 0.0001f) {
        return;
    }

    camera = renderer_get_camera(renderer);
    if (camera == NULL) {
        return;
    }
    game_get_camera_viewport_size(renderer, &viewport_width, &viewport_height);
    if (viewport_width <= 0.0f || viewport_height <= 0.0f) {
        return;
    }

    zoom_factor = powf(1.0f - CAMERA_ZOOM_STEP, zoom_steps);
    old_view_width = camera->view_width;
    old_view_height = camera->view_height;
    new_view_width = old_view_width * zoom_factor;
    new_view_height = old_view_height * zoom_factor;

    if (new_view_width < CAMERA_MIN_VIEW_WIDTH) {
        zoom_factor = CAMERA_MIN_VIEW_WIDTH / old_view_width;
        new_view_width = CAMERA_MIN_VIEW_WIDTH;
        new_view_height = old_view_height * zoom_factor;
    }
    if (new_view_height < CAMERA_MIN_VIEW_HEIGHT) {
        zoom_factor = CAMERA_MIN_VIEW_HEIGHT / old_view_height;
        new_view_height = CAMERA_MIN_VIEW_HEIGHT;
        new_view_width = old_view_width * zoom_factor;
    }
    if (new_view_width > WORLD_WIDTH) {
        zoom_factor = WORLD_WIDTH / old_view_width;
        new_view_width = WORLD_WIDTH;
        new_view_height = old_view_height * zoom_factor;
    }
    if (new_view_height > WORLD_HEIGHT) {
        zoom_factor = WORLD_HEIGHT / old_view_height;
        new_view_height = WORLD_HEIGHT;
        new_view_width = old_view_width * zoom_factor;
    }

    anchor_u = clamp_f(anchor_screen.x / viewport_width, 0.0f, 1.0f);
    anchor_v = clamp_f(anchor_screen.y / viewport_height, 0.0f, 1.0f);
    anchor_world.x = camera->x + anchor_u * old_view_width;
    anchor_world.y = camera->y + anchor_v * old_view_height;
    camera->view_width = new_view_width;
    camera->view_height = new_view_height;
    camera->x = anchor_world.x - anchor_u * new_view_width;
    camera->y = anchor_world.y - anchor_v * new_view_height;
    game_render_clamp_camera(camera);
}

size_t game_pick_ball_from_snapshot(
    const RenderSnapshotBuffer* render_snapshot,
    const Camera* camera,
    Vec2 screen_point
) {
    Vec2 world_point;
    size_t index;

    if (render_snapshot == NULL || camera == NULL) {
        return INVALID_INDEX;
    }

    world_point = camera_screen_to_world(camera, screen_point);
    for (index = render_snapshot->world.pick_target_count; index > 0U; --index) {
        const PickTargetView* target = &render_snapshot->world.pick_targets[index - 1U];
        if (target->is_circle) {
            float dx = world_point.x - target->x;
            float dy = world_point.y - target->y;
            if ((dx * dx) + (dy * dy) <= target->radius * target->radius) {
                return (size_t)(target->id - 1U);
            }
        } else {
            if (world_point.x >= target->x - target->extent_x &&
                world_point.x <= target->x + target->extent_x &&
                world_point.y >= target->y - target->extent_y &&
                world_point.y <= target->y + target->extent_y) {
                return (size_t)(target->id - 1U);
            }
        }
    }

    return INVALID_INDEX;
}

void game_update_render_interactions(
    GameState* game,
    Renderer* renderer,
    const EngineInput* input,
    const RenderSnapshotBuffer* render_snapshot,
    bool pointer_over_ui,
    double dt_seconds
) {
    float viewport_width = WORLD_WIDTH;
    float viewport_height = WORLD_HEIGHT;
    float pan_scale_x = 1.0f;
    float pan_scale_y = 1.0f;
    float pan_dx = 0.0f;
    float pan_dy = 0.0f;
    float zoom_steps = 0.0f;
    float drag_distance = 0.0f;
    float dt = 0.0f;
    bool pan_key_left = false;
    bool pan_key_right = false;
    bool pan_key_up = false;
    bool pan_key_down = false;
    bool zoom_in_down = false;
    bool zoom_out_down = false;
    bool need_mouse_world = false;
    bool camera_rect_dirty = true;
    Vec2 anchor_screen = {0.0f, 0.0f};
    Vec2 mouse_screen = {0.0f, 0.0f};
    Vec2 mouse_world = {0.0f, 0.0f};
    bool hover_query_dirty = true;
    Camera* camera = NULL;

    if (game == NULL || renderer == NULL || input == NULL) {
        return;
    }

    camera = renderer_get_camera(renderer);
    if (camera == NULL) {
        return;
    }
    game_get_camera_viewport_size(renderer, &viewport_width, &viewport_height);
    camera->viewport_width = viewport_width;
    camera->viewport_height = viewport_height;
    if (viewport_width > 0.0f) {
        pan_scale_x = camera->view_width / viewport_width;
    }
    if (viewport_height > 0.0f) {
        pan_scale_y = camera->view_height / viewport_height;
    }
    mouse_screen.x = (float)input->mouse_x;
    mouse_screen.y = (float)input->mouse_y;
    camera_rect_dirty = game_camera_rect_changed(game, camera);
    if (camera_rect_dirty) {
        game->camera_rect_dirty_count += 1U;
    } else {
        game->camera_rect_stable_count += 1U;
    }
    hover_query_dirty =
        !game->hover_cache_valid ||
        render_snapshot == NULL ||
        render_snapshot->sequence != game->last_hover_snapshot_sequence ||
        input->mouse_x != game->last_hover_mouse_x ||
        input->mouse_y != game->last_hover_mouse_y ||
        pointer_over_ui != game->last_hover_pointer_over_ui ||
        camera_rect_dirty;
    if (hover_query_dirty) {
        game->hover_query_dirty_count += 1U;
        game->hovered_ball_index = pointer_over_ui ? INVALID_INDEX : game_pick_ball_from_snapshot(render_snapshot, camera, mouse_screen);
        game->hover_pick_query_count += 1U;
        game->last_hover_snapshot_sequence = render_snapshot != NULL ? render_snapshot->sequence : 0U;
        game->last_hover_mouse_x = input->mouse_x;
        game->last_hover_mouse_y = input->mouse_y;
        game->last_hover_pointer_over_ui = pointer_over_ui;
        game->last_hover_camera_x = camera->x;
        game->last_hover_camera_y = camera->y;
        game->last_hover_camera_view_width = camera->view_width;
        game->last_hover_camera_view_height = camera->view_height;
        game->hover_cache_valid = true;
    } else {
        game->hover_pick_skip_count += 1U;
    }
    dt = dt_seconds > 0.0001f ? (float)dt_seconds : (1.0f / 240.0f);

    if (EngineInput_was_mouse_pressed(input, 1U) && !pointer_over_ui) {
        if (game->hovered_ball_index != INVALID_INDEX) {
            need_mouse_world = true;
            mouse_world = camera_screen_to_world(camera, mouse_screen);
            game->entity_drag_active = true;
            game->dragged_ball_index = game->hovered_ball_index;
            game->drag_world_position = mouse_world;
            game->drag_release_velocity = (Vec2){ 0.0f, 0.0f };
            game->selected_ball_index = game->hovered_ball_index;
            game->hover_cache_valid = false;
        } else {
            game->camera_pan_active = true;
            game->camera_pan_start_mouse_x = input->mouse_x;
            game->camera_pan_start_mouse_y = input->mouse_y;
            game->camera_pan_start_x = camera->x;
            game->camera_pan_start_y = camera->y;
        }
    }

    if (game->entity_drag_active) {
        if (EngineInput_is_mouse_down(input, 1U)) {
            if (!need_mouse_world) {
                mouse_world = camera_screen_to_world(camera, mouse_screen);
                need_mouse_world = true;
            }
            game->drag_release_velocity.x = (mouse_world.x - game->drag_world_position.x) / dt;
            game->drag_release_velocity.y = (mouse_world.y - game->drag_world_position.y) / dt;
            game->drag_world_position = mouse_world;
            game->hovered_ball_index = game->dragged_ball_index;
            game->hover_cache_valid = false;
        } else {
            game->entity_drag_active = false;
            game->hover_cache_valid = false;
        }
    }

    if (game->camera_pan_active) {
        if (EngineInput_is_mouse_down(input, 1U)) {
            pan_dx = (float)(input->mouse_x - game->camera_pan_start_mouse_x) * pan_scale_x;
            pan_dy = (float)(input->mouse_y - game->camera_pan_start_mouse_y) * pan_scale_y;
            camera->x = game->camera_pan_start_x - pan_dx;
            camera->y = game->camera_pan_start_y - pan_dy;
            game_render_clamp_camera(camera);
        } else {
            float dx = (float)(input->mouse_x - game->camera_pan_start_mouse_x);
            float dy = (float)(input->mouse_y - game->camera_pan_start_mouse_y);
            drag_distance = sqrtf(dx * dx + dy * dy);
            if (drag_distance <= CAMERA_PAN_CLICK_THRESHOLD && !pointer_over_ui) {
                game->selected_ball_index = game->hovered_ball_index;
            }
            game->camera_pan_active = false;
            game->hover_cache_valid = false;
        }
    }

    pan_key_left = EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_LEFT);
    pan_key_right = EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_RIGHT);
    pan_key_up = EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_UP);
    pan_key_down = EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_DOWN);
    pan_dx = 0.0f;
    pan_dy = 0.0f;
    if (pan_key_left) {
        pan_dx -= CAMERA_PAN_KEY_SPEED * (float)dt_seconds;
    }
    if (pan_key_right) {
        pan_dx += CAMERA_PAN_KEY_SPEED * (float)dt_seconds;
    }
    if (pan_key_up) {
        pan_dy -= CAMERA_PAN_KEY_SPEED * (float)dt_seconds;
    }
    if (pan_key_down) {
        pan_dy += CAMERA_PAN_KEY_SPEED * (float)dt_seconds;
    }
    if (pan_dx != 0.0f || pan_dy != 0.0f) {
        camera->x += pan_dx;
        camera->y += pan_dy;
        game_render_clamp_camera(camera);
        game->hover_cache_valid = false;
    }

    if (!pointer_over_ui && !game->camera_pan_active && !game->entity_drag_active) {
        zoom_in_down = EngineInput_is_down(input, ENGINE_INPUT_ACTION_ZOOM_IN);
        zoom_out_down = EngineInput_is_down(input, ENGINE_INPUT_ACTION_ZOOM_OUT);
        zoom_steps += input->mouse_wheel_delta;
        if (zoom_in_down) {
            zoom_steps += (float)dt_seconds * CAMERA_ZOOM_KEY_SPEED;
        }
        if (zoom_out_down) {
            zoom_steps -= (float)dt_seconds * CAMERA_ZOOM_KEY_SPEED;
        }
        if (fabsf(zoom_steps) > 0.0001f) {
            anchor_screen.x = (float)input->mouse_x;
            anchor_screen.y = (float)input->mouse_y;
            game_apply_render_camera_zoom(renderer, zoom_steps, anchor_screen);
            game->hover_cache_valid = false;
        }
    }

    game_sync_ball_highlight_state(game);
}

void game_apply_entity_drag_packet(GameState* game, const GameInputPacket* input_packet) {
    PhysicsWorld* physics_world = NULL;
    Ball* ball;
    Entity* entity;
    RigidBodyComponent* rigid_body;
    TransformComponent* transform;

    if (game == NULL || input_packet == NULL || game->scene == NULL) {
        return;
    }
    physics_world = Scene_GetPhysicsWorld(game->scene);
    if (physics_world == NULL) {
        return;
    }
    if ((!input_packet->drag_entity_active && !input_packet->drag_entity_release) ||
        input_packet->drag_ball_index >= BALL_COUNT) {
        return;
    }

    ball = &game->balls[input_packet->drag_ball_index];
    entity = ball->entity;
    rigid_body = ball->rigid_body;
    transform = ball->transform;
    if (entity == NULL || rigid_body == NULL || transform == NULL) {
        return;
    }

    if (input_packet->drag_entity_active) {
        if (rigid_body->has_body) {
            PhysicsWorld_SetEntityAwake(physics_world, entity, true);
        }
        PhysicsWorld_SetEntityPosition(
            physics_world,
            entity,
            input_packet->drag_world_x,
            input_packet->drag_world_y
        );
        if (rigid_body->has_body) {
            PhysicsWorld_SetEntityLinearVelocity(physics_world, entity, (Vec2){ 0.0f, 0.0f });
            PhysicsWorld_SetEntityAngularVelocity(physics_world, entity, 0.0f);
        }
    }

    if (input_packet->drag_entity_release && rigid_body->has_body) {
        PhysicsWorld_SetEntityAwake(physics_world, entity, true);
        PhysicsWorld_SetEntityLinearVelocity(
            physics_world,
            entity,
            (Vec2){
                input_packet->drag_linear_velocity_x * 0.55f,
                input_packet->drag_linear_velocity_y * 0.55f
            }
        );
    }
}
