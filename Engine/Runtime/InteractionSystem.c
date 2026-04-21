#include "InteractionSystem.h"

#include "../Rendering/Camera.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RenderSnapshot.h"
#include "../Settings.h"

#include <math.h>
#include <string.h>

void InteractionSystem_ConfigDefaults(InteractionSystemConfig* config)
{
    const EngineSettings* settings = EngineSettings_GetDefaults();

    if (config == NULL)
    {
        return;
    }

    config->camera_pan_key_speed = settings->camera_pan_key_speed;
    config->camera_zoom_step = settings->camera_zoom_step;
    config->camera_zoom_key_speed = settings->camera_zoom_key_speed;
    config->camera_pan_click_threshold = settings->camera_pan_click_threshold;
    config->camera_min_view_width = settings->camera_min_view_width;
    config->camera_min_view_height = settings->camera_min_view_height;
}

void InteractionSystem_Init(InteractionSystemState* state)
{
    if (state == NULL)
    {
        return;
    }

    memset(state, 0, sizeof(*state));
}

static bool interaction_camera_rect_changed(const InteractionSystemState* state, const Camera* camera)
{
    if (state == NULL || camera == NULL)
    {
        return true;
    }

    return !state->hover_cache_valid ||
           camera->x != state->last_hover_camera_x ||
           camera->y != state->last_hover_camera_y ||
           camera->view_width != state->last_hover_camera_view_width ||
           camera->view_height != state->last_hover_camera_view_height;
}

void InteractionSystem_UpdateRender(
    InteractionSystemState* state,
    const InteractionSystemConfig* config,
    Renderer* renderer,
    const EngineInput* input,
    const RenderSnapshotBuffer* render_snapshot,
    bool pointer_over_ui,
    double dt_seconds
)
{
    InteractionSystemConfig defaults;
    const InteractionSystemConfig* resolved_config = config;
    Camera* camera;
    int viewport_width_i = 0;
    int viewport_height_i = 0;
    float viewport_width = 1.0f;
    float viewport_height = 1.0f;
    float pan_scale_x = 1.0f;
    float pan_scale_y = 1.0f;
    float pan_dx = 0.0f;
    float pan_dy = 0.0f;
    float zoom_steps = 0.0f;
    float dt;
    bool camera_rect_dirty;
    bool hover_query_dirty;
    Vec2 mouse_screen;
    Vec2 mouse_world = { 0.0f, 0.0f };
    bool need_mouse_world = false;

    if (state == NULL || renderer == NULL || input == NULL)
    {
        return;
    }

    if (resolved_config == NULL)
    {
        InteractionSystem_ConfigDefaults(&defaults);
        resolved_config = &defaults;
    }

    camera = renderer_get_camera(renderer);
    if (camera == NULL)
    {
        return;
    }

    renderer_get_viewport_size(renderer, &viewport_width_i, &viewport_height_i);
    if (viewport_width_i > 0) viewport_width = (float)viewport_width_i;
    if (viewport_height_i > 0) viewport_height = (float)viewport_height_i;
    camera->viewport_width = viewport_width;
    camera->viewport_height = viewport_height;
    pan_scale_x = camera->view_width / viewport_width;
    pan_scale_y = camera->view_height / viewport_height;
    mouse_screen.x = (float)input->mouse_x;
    mouse_screen.y = (float)input->mouse_y;
    camera_rect_dirty = interaction_camera_rect_changed(state, camera);
    if (camera_rect_dirty)
    {
        state->camera_rect_dirty_count += 1U;
    }
    else
    {
        state->camera_rect_stable_count += 1U;
    }

    hover_query_dirty =
        !state->hover_cache_valid ||
        render_snapshot == NULL ||
        render_snapshot_buffer_get_sequence(render_snapshot) != state->last_hover_snapshot_sequence ||
        input->mouse_x != state->last_hover_mouse_x ||
        input->mouse_y != state->last_hover_mouse_y ||
        pointer_over_ui != state->last_hover_pointer_over_ui ||
        camera_rect_dirty;
    if (hover_query_dirty)
    {
        state->hover_query_dirty_count += 1U;
        state->hovered_entity_id = pointer_over_ui ? 0U : render_snapshot_buffer_pick_entity(render_snapshot, camera, mouse_screen);
        state->hover_pick_query_count += 1U;
        state->last_hover_snapshot_sequence = render_snapshot_buffer_get_sequence(render_snapshot);
        state->last_hover_mouse_x = input->mouse_x;
        state->last_hover_mouse_y = input->mouse_y;
        state->last_hover_pointer_over_ui = pointer_over_ui;
        state->last_hover_camera_x = camera->x;
        state->last_hover_camera_y = camera->y;
        state->last_hover_camera_view_width = camera->view_width;
        state->last_hover_camera_view_height = camera->view_height;
        state->hover_cache_valid = true;
    }
    else
    {
        state->hover_pick_skip_count += 1U;
    }

    dt = dt_seconds > 0.0001 ? (float)dt_seconds : (1.0f / 240.0f);
    if (EngineInput_was_mouse_pressed(input, 1U) && !pointer_over_ui)
    {
        if (state->hovered_entity_id != 0U)
        {
            need_mouse_world = true;
            mouse_world = camera_screen_to_world(camera, mouse_screen);
            state->entity_drag_active = true;
            state->dragged_entity_id = state->hovered_entity_id;
            state->drag_world_position = mouse_world;
            state->drag_release_velocity = (Vec2){ 0.0f, 0.0f };
            state->selected_entity_id = state->hovered_entity_id;
            state->hover_cache_valid = false;
        }
        else
        {
            state->camera_pan_active = true;
            state->camera_pan_start_mouse_x = input->mouse_x;
            state->camera_pan_start_mouse_y = input->mouse_y;
            state->camera_pan_start_x = camera->x;
            state->camera_pan_start_y = camera->y;
        }
    }

    if (state->entity_drag_active)
    {
        if (EngineInput_is_mouse_down(input, 1U))
        {
            if (!need_mouse_world)
            {
                mouse_world = camera_screen_to_world(camera, mouse_screen);
                need_mouse_world = true;
            }
            state->drag_release_velocity.x = (mouse_world.x - state->drag_world_position.x) / dt;
            state->drag_release_velocity.y = (mouse_world.y - state->drag_world_position.y) / dt;
            state->drag_world_position = mouse_world;
            state->hovered_entity_id = state->dragged_entity_id;
            state->hover_cache_valid = false;
        }
        else
        {
            state->entity_drag_active = false;
            state->hover_cache_valid = false;
        }
    }

    if (state->camera_pan_active)
    {
        if (EngineInput_is_mouse_down(input, 1U))
        {
            pan_dx = (float)(input->mouse_x - state->camera_pan_start_mouse_x) * pan_scale_x;
            pan_dy = (float)(input->mouse_y - state->camera_pan_start_mouse_y) * pan_scale_y;
            camera->x = state->camera_pan_start_x - pan_dx;
            camera->y = state->camera_pan_start_y - pan_dy;
            camera_clamp_to_bounds(camera, resolved_config->camera_min_view_width, resolved_config->camera_min_view_height);
        }
        else
        {
            float dx = (float)(input->mouse_x - state->camera_pan_start_mouse_x);
            float dy = (float)(input->mouse_y - state->camera_pan_start_mouse_y);
            float distance = sqrtf(dx * dx + dy * dy);
            if (distance <= resolved_config->camera_pan_click_threshold && !pointer_over_ui)
            {
                state->selected_entity_id = state->hovered_entity_id;
            }
            state->camera_pan_active = false;
            state->hover_cache_valid = false;
        }
    }

    if (EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_LEFT)) pan_dx -= resolved_config->camera_pan_key_speed * (float)dt_seconds;
    if (EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_RIGHT)) pan_dx += resolved_config->camera_pan_key_speed * (float)dt_seconds;
    if (EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_UP)) pan_dy -= resolved_config->camera_pan_key_speed * (float)dt_seconds;
    if (EngineInput_is_down(input, ENGINE_INPUT_ACTION_PAN_DOWN)) pan_dy += resolved_config->camera_pan_key_speed * (float)dt_seconds;
    if (pan_dx != 0.0f || pan_dy != 0.0f)
    {
        camera->x += pan_dx;
        camera->y += pan_dy;
        camera_clamp_to_bounds(camera, resolved_config->camera_min_view_width, resolved_config->camera_min_view_height);
        state->hover_cache_valid = false;
    }

    if (!pointer_over_ui && !state->camera_pan_active && !state->entity_drag_active)
    {
        zoom_steps += input->mouse_wheel_delta;
        if (EngineInput_is_down(input, ENGINE_INPUT_ACTION_ZOOM_IN)) zoom_steps += (float)dt_seconds * resolved_config->camera_zoom_key_speed;
        if (EngineInput_is_down(input, ENGINE_INPUT_ACTION_ZOOM_OUT)) zoom_steps -= (float)dt_seconds * resolved_config->camera_zoom_key_speed;
        if (fabsf(zoom_steps) > 0.0001f)
        {
            float zoom_factor = powf(1.0f - resolved_config->camera_zoom_step, zoom_steps);
            camera_zoom_at_screen_point(
                camera,
                zoom_factor,
                mouse_screen,
                resolved_config->camera_min_view_width,
                resolved_config->camera_min_view_height
            );
            state->hover_cache_valid = false;
        }
    }
}
