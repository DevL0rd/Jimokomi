#include "InteractionSystem.h"

#include "../Rendering/Camera.h"
#include "../Rendering/Renderer.h"

#include <string.h>

void InteractionSystem_WriteInputPacket(
    const InteractionSystemState* state,
    const EngineInputSnapshot* input_snapshot,
    const Renderer* renderer,
    bool pointer_over_ui,
    bool debug_overlay_enabled,
    bool draw_debug_world,
    int window_width,
    int window_height,
    uint64_t frame_id,
    EngineRuntimeInputPacket* out_packet
)
{
    const Camera* camera;

    if (state == NULL || input_snapshot == NULL || out_packet == NULL)
    {
        return;
    }

    memset(out_packet, 0, sizeof(*out_packet));
    out_packet->snapshot = *input_snapshot;
    out_packet->pointer_over_ui = pointer_over_ui;
    out_packet->debug_overlay_enabled = debug_overlay_enabled;
    out_packet->draw_debug_world = draw_debug_world;
    out_packet->drag_entity_active = state->entity_drag_active && state->dragged_entity_id != 0U;
    out_packet->drag_entity_release = !state->entity_drag_active && state->dragged_entity_id != 0U &&
        (input_snapshot->mouse_buttons & 1U) == 0U;
    out_packet->drag_entity_id = (uint32_t)state->dragged_entity_id;
    out_packet->drag_world_x = state->drag_world_position.x;
    out_packet->drag_world_y = state->drag_world_position.y;
    out_packet->drag_linear_velocity_x = state->drag_release_velocity.x;
    out_packet->drag_linear_velocity_y = state->drag_release_velocity.y;
    out_packet->window_width = window_width;
    out_packet->window_height = window_height;
    out_packet->selected_entity_id = state->selected_entity_id;
    out_packet->hovered_entity_id = state->hovered_entity_id;
    out_packet->frame_id = frame_id;
    camera = renderer_get_camera_const(renderer);
    if (camera != NULL)
    {
        out_packet->camera_x = camera->x;
        out_packet->camera_y = camera->y;
        out_packet->camera_view_width = camera->view_width;
        out_packet->camera_view_height = camera->view_height;
    }
}

void InteractionSystem_ClearReleasedDrag(InteractionSystemState* state, const EngineInput* input)
{
    if (state == NULL || input == NULL)
    {
        return;
    }

    if (!state->entity_drag_active && EngineInput_was_mouse_released(input, 1U))
    {
        state->dragged_entity_id = 0U;
        state->drag_release_velocity = (Vec2){ 0.0f, 0.0f };
    }
}
