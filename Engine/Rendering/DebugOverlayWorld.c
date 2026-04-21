#include "DebugOverlayInternal.h"

#include <math.h>

static void debug_draw_entity(
    Target* target,
    const Camera* camera,
    const DebugEntityView* entity,
    bool is_selected,
    bool is_hovered
) {
    Vec2 screen_position;
    Vec2 screen_extent;
    Vec2 heading;
    Vec2 right;
    Vec2 velocity;
    float speed;
    float velocity_scale;
    Color32 outline = { 0x49ffaeU };
    Color32 heading_color = { 0x7ee0ffU };
    Color32 velocity_color = { 0xffd166U };

    if (target == NULL || camera == NULL || entity == NULL) {
        return;
    }

    if (is_selected) {
        outline.value = 0x00ffffU;
    } else if (is_hovered) {
        outline.value = 0x8de6ffU;
    } else if (entity->is_sleeping) {
        outline.value = 0x6b7890U;
    }

    screen_position = camera_world_to_screen(camera, entity->position);
    screen_extent = camera_world_size_to_screen(
        camera,
        (Vec2){
            entity->is_circle ? entity->radius : entity->extent_x,
            entity->is_circle ? entity->radius : entity->extent_y
        }
    );
    if (entity->is_circle) {
        target_circle(target, screen_position, screen_extent.x, outline);
    } else {
        target_rect(target, (Rect){
            screen_position.x - screen_extent.x,
            screen_position.y - screen_extent.y,
            screen_extent.x * 2.0f,
            screen_extent.y * 2.0f
        }, outline);
    }

    heading.x = cosf(entity->angle_radians);
    heading.y = sinf(entity->angle_radians);
    right.x = -heading.y;
    right.y = heading.x;
    target_line(
        target,
        screen_position.x,
        screen_position.y,
        screen_position.x + heading.x * 16.0f,
        screen_position.y + heading.y * 16.0f,
        heading_color
    );
    target_line(
        target,
        screen_position.x - right.x * 6.0f,
        screen_position.y - right.y * 6.0f,
        screen_position.x + right.x * 6.0f,
        screen_position.y + right.y * 6.0f,
        heading_color
    );

    speed = sqrtf(entity->velocity.x * entity->velocity.x + entity->velocity.y * entity->velocity.y);
    velocity_scale = clamp_f(speed * 0.55f, 0.0f, 20.0f);
    velocity.x = speed > 0.001f ? (entity->velocity.x / speed) * velocity_scale : 0.0f;
    velocity.y = speed > 0.001f ? (entity->velocity.y / speed) * velocity_scale : 0.0f;
    target_line(
        target,
        screen_position.x,
        screen_position.y,
        screen_position.x + velocity.x,
        screen_position.y + velocity.y,
        velocity_color
    );
}

static bool debug_entity_visible(const Camera* camera, const DebugEntityView* entity) {
    float extent_x;
    float extent_y;

    if (camera == NULL || entity == NULL) {
        return false;
    }

    extent_x = entity->is_circle ? entity->radius : entity->extent_x;
    extent_y = entity->is_circle ? entity->radius : entity->extent_y;

    return entity->position.x + extent_x >= camera->x &&
           entity->position.y + extent_y >= camera->y &&
           entity->position.x - extent_x <= camera->x + camera->view_width &&
           entity->position.y - extent_y <= camera->y + camera->view_height;
}

static void debug_draw_spatial_grid(
    Target* target,
    const Camera* camera,
    const DebugGridView* grid
) {
    int min_cell_x;
    int max_cell_x;
    int min_cell_y;
    int max_cell_y;
    int cell_x;
    int cell_y;

    if (target == NULL || camera == NULL || grid == NULL || !grid->enabled || grid->cell_size <= 0.0f) {
        return;
    }

    min_cell_x = clamp_i((int)floorf((camera->x - grid->min_x) / grid->cell_size), 0, grid->span_max_x);
    max_cell_x = clamp_i((int)ceilf((camera->x + camera->view_width - grid->min_x) / grid->cell_size), 0, grid->span_max_x);
    min_cell_y = clamp_i((int)floorf((camera->y - grid->min_y) / grid->cell_size), 0, grid->span_max_y);
    max_cell_y = clamp_i((int)ceilf((camera->y + camera->view_height - grid->min_y) / grid->cell_size), 0, grid->span_max_y);

    for (cell_x = min_cell_x; cell_x <= max_cell_x + 1; ++cell_x) {
        float world_x = grid->min_x + (float)cell_x * grid->cell_size;
        Vec2 screen_a = camera_world_to_screen(camera, (Vec2){ world_x, camera->y });
        Vec2 screen_b = camera_world_to_screen(camera, (Vec2){ world_x, camera->y + camera->view_height });
        Color32 color = (Color32){ 0x17303cU };
        if (cell_x == grid->span_min_x || cell_x == grid->span_max_x + 1) {
            color = (Color32){ 0x4bb8ccU };
        }
        target_line(target, screen_a.x, screen_a.y, screen_b.x, screen_b.y, color);
    }

    for (cell_y = min_cell_y; cell_y <= max_cell_y + 1; ++cell_y) {
        float world_y = grid->min_y + (float)cell_y * grid->cell_size;
        Vec2 screen_a = camera_world_to_screen(camera, (Vec2){ camera->x, world_y });
        Vec2 screen_b = camera_world_to_screen(camera, (Vec2){ camera->x + camera->view_width, world_y });
        Color32 color = (Color32){ 0x17303cU };
        if (cell_y == grid->span_min_y || cell_y == grid->span_max_y + 1) {
            color = (Color32){ 0x4bb8ccU };
        }
        target_line(target, screen_a.x, screen_a.y, screen_b.x, screen_b.y, color);
    }
}

static uint64_t debug_overlay_compute_world_signature(
    const DebugOverlay* overlay,
    const Camera* camera,
    const DebugGridView* grid,
    const DebugEntityView* entities,
    size_t entity_count,
    const DebugCollisionView* collisions,
    size_t collision_count,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id
) {
    uint64_t hash = 1469598103934665603ULL;
    size_t index = 0U;

    if (overlay == NULL || camera == NULL) {
        return 0ULL;
    }

    hash = debug_hash_i32(hash, debug_round_tenths(camera->x));
    hash = debug_hash_i32(hash, debug_round_tenths(camera->y));
    hash = debug_hash_i32(hash, debug_round_tenths(camera->view_width));
    hash = debug_hash_i32(hash, debug_round_tenths(camera->view_height));
    hash = debug_hash_u64(hash, selected_entity_id);
    hash = debug_hash_u64(hash, hovered_entity_id);
    hash = debug_hash_u64(hash, entity_count);
    hash = debug_hash_u64(hash, collision_count);
    if (grid != NULL && grid->enabled) {
        hash = debug_hash_i32(hash, debug_round_tenths(grid->cell_size));
        hash = debug_hash_i32(hash, grid->span_min_x);
        hash = debug_hash_i32(hash, grid->span_max_x);
        hash = debug_hash_i32(hash, grid->span_min_y);
        hash = debug_hash_i32(hash, grid->span_max_y);
    }

    for (index = 0U; index < entity_count; ++index) {
        const DebugEntityView* entity = &entities[index];
        hash = debug_hash_u64(hash, entity->id);
        hash = debug_hash_i32(hash, debug_round_tenths(entity->position.x));
        hash = debug_hash_i32(hash, debug_round_tenths(entity->position.y));
        hash = debug_hash_i32(hash, debug_round_tenths(entity->angle_radians));
        hash = debug_hash_u64(hash, entity->is_sleeping ? 1U : 0U);
    }

    for (index = 0U; index < collision_count; ++index) {
        const DebugCollisionView* collision = &collisions[index];
        if (!collision->active) {
            continue;
        }
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_point.x));
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_point.y));
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_normal.x));
        hash = debug_hash_i32(hash, debug_round_tenths(collision->contact_normal.y));
        hash = debug_hash_u64(hash, collision->sensor ? 1U : 0U);
        hash = debug_hash_u64(hash, collision->sleeping ? 1U : 0U);
    }

    return hash;
}

void debug_overlay_draw_world(
    DebugOverlay* overlay,
    RenderBackend* backend,
    uint64_t now_ms,
    const Camera* camera,
    const DebugGridView* grid,
    const DebugEntityView* entities,
    size_t entity_count,
    const DebugCollisionView* collisions,
    size_t collision_count,
    uint64_t selected_entity_id,
    uint64_t hovered_entity_id
) {
    size_t index;
    Target target;
    bool has_selected_entity;
    bool has_hovered_entity;
    uint64_t world_signature;
    double redraw_started_ms = 0.0;
    (void)now_ms;

    if (overlay == NULL || !overlay->draw_world_gizmos || backend == NULL || camera == NULL) {
        return;
    }

    if (backend->create_surface == NULL ||
        backend->destroy_surface == NULL ||
        backend->set_target == NULL ||
        backend->reset_target == NULL ||
        backend->clear == NULL) {
        return;
    }

    if (overlay->world_surface == NULL ||
        overlay->world_surface_width != (int)camera->viewport_width ||
        overlay->world_surface_height != (int)camera->viewport_height) {
        if (overlay->world_surface != NULL) {
            backend->destroy_surface(backend->userdata, overlay->world_surface);
        }
        overlay->world_surface = backend->create_surface(
            backend->userdata,
            (int)camera->viewport_width,
            (int)camera->viewport_height
        );
        overlay->world_surface_backend = backend;
        overlay->world_surface_width = (int)camera->viewport_width;
        overlay->world_surface_height = (int)camera->viewport_height;
    }

    if (overlay->world_surface == NULL) {
        return;
    }

    world_signature = debug_overlay_compute_world_signature(
        overlay,
        camera,
        grid,
        entities,
        entity_count,
        collisions,
        collision_count,
        selected_entity_id,
        hovered_entity_id
    );
    overlay->last_visible_entity_count = 0U;
    overlay->last_active_collision_count = 0U;
    has_selected_entity = selected_entity_id != 0U;
    has_hovered_entity = hovered_entity_id != 0U;

    if (overlay->last_world_signature != world_signature) {
        redraw_started_ms = debug_overlay_now_ms();
        backend->set_target(backend->userdata, overlay->world_surface);
        backend->clear(backend->userdata, (Color32){ 0x00000000U });
        target_init(&target, backend, 0.0f, 0.0f);

        debug_draw_spatial_grid(&target, camera, grid);

        for (index = 0U; index < entity_count; ++index) {
            if (!debug_entity_visible(camera, &entities[index])) {
                continue;
            }
            overlay->last_visible_entity_count += 1U;
            debug_draw_entity(
                &target,
                camera,
                &entities[index],
                has_selected_entity && entities[index].id == selected_entity_id,
                has_hovered_entity && entities[index].id == hovered_entity_id
            );
        }
        for (index = 0U; index < collision_count; ++index) {
            Vec2 a;
            Vec2 b;
            Vec2 c;
            Vec2 n;
            Color32 color;
            if (!collisions[index].active) {
                continue;
            }
            overlay->last_active_collision_count += 1U;
            a = camera_world_to_screen(camera, collisions[index].point_a);
            b = camera_world_to_screen(camera, collisions[index].point_b);
            c = camera_world_to_screen(camera, collisions[index].contact_point);
            n = collisions[index].contact_normal;
            color.value = collisions[index].sleeping ? 0x666666U : (collisions[index].sensor ? 0xff00ffU : 0xffff00U);
            target_line(&target, a.x, a.y, b.x, b.y, color);
            target_circle_filled(&target, c, camera_world_size_to_screen(camera, (Vec2){ 1.5f, 1.5f }).x, color);
            target_line(&target, c.x, c.y, c.x + n.x * 8.0f, c.y + n.y * 8.0f, color);
        }

        backend->reset_target(backend->userdata);
        overlay->last_world_signature = world_signature;
        overlay->world_redraw_count += 1U;
        overlay->last_world_redraw_ms = debug_overlay_now_ms() - redraw_started_ms;
    } else {
        overlay->world_redraw_skip_count += 1U;
    }

    target_init(&target, backend, 0.0f, 0.0f);
    target_surface(&target, overlay->world_surface, 0.0f, 0.0f);
}
