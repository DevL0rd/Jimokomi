#include "RenderSnapshot.h"
#include "RenderSnapshotExchange.h"
#include "RenderSnapshotInternal.h"
#include "../Core/Profiling.h"

#include "../Core/Memory.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

struct RenderSnapshotExchange {
    RenderSnapshotBuffer buffers[3];
    size_t write_index;
    atomic_size_t published_index;
    atomic_uint_fast64_t published_sequence;
    atomic_uint reader_counts[3];
};

static void render_snapshot_exchange_dispose(RenderSnapshotExchange* exchange);

static bool render_snapshot_exchange_alloc_world(
    RenderWorldSnapshot* world
) {
    if (world == NULL) {
        return false;
    }

    memset(world, 0, sizeof(*world));
    return true;
}

static void render_snapshot_exchange_free_world(RenderWorldSnapshot* world) {
    if (world == NULL) {
        return;
    }

    free(world->material_renderables);
    free(world->procedural_meshes);
    free(world->triangles);
    free(world->lines);
    free(world->particle_surface_mesh_inputs);
    free(world->particle_surface_mesh_particles);
    free(world->debug_entities);
    free(world->debug_collisions);
    free(world->pick_targets);
    memset(world, 0, sizeof(*world));
}

static bool render_snapshot_exchange_init(RenderSnapshotExchange* exchange) {
    size_t index;

    if (exchange == NULL) {
        return false;
    }

    memset(exchange, 0, sizeof(*exchange));
    atomic_init(&exchange->published_index, 0U);
    atomic_init(&exchange->published_sequence, 0U);
    for (index = 0U; index < 3U; ++index) {
        atomic_init(&exchange->reader_counts[index], 0U);
    }

    for (index = 0U; index < 3U; ++index) {
        if (!render_snapshot_exchange_alloc_world(&exchange->buffers[index].world)) {
            render_snapshot_exchange_dispose(exchange);
            return false;
        }
    }

    return true;
}

bool render_world_snapshot_reserve_material_renderables(RenderWorldSnapshot* snapshot, size_t required_capacity) {
    MaterialRenderable* next_items;
    DebugEntityView* next_debug_entities;
    PickTargetView* next_pick_targets;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->material_renderable_capacity >= required_capacity &&
        snapshot->debug_entity_capacity >= required_capacity &&
        snapshot->pick_target_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->material_renderable_capacity > 0U ? snapshot->material_renderable_capacity : 64U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_items = (MaterialRenderable*)realloc(snapshot->material_renderables, next_capacity * sizeof(*next_items));
    if (next_items == NULL) {
        return false;
    }
    snapshot->material_renderables = next_items;
    snapshot->material_renderable_capacity = next_capacity;

    next_debug_entities = (DebugEntityView*)realloc(snapshot->debug_entities, next_capacity * sizeof(*next_debug_entities));
    if (next_debug_entities == NULL) {
        return false;
    }
    snapshot->debug_entities = next_debug_entities;
    snapshot->debug_entity_capacity = next_capacity;

    next_pick_targets = (PickTargetView*)realloc(snapshot->pick_targets, next_capacity * sizeof(*next_pick_targets));
    if (next_pick_targets == NULL) {
        return false;
    }
    snapshot->pick_targets = next_pick_targets;
    snapshot->pick_target_capacity = next_capacity;
    return true;
}

bool render_world_snapshot_reserve_procedural_meshes(RenderWorldSnapshot* snapshot, size_t required_capacity) {
    ProceduralMeshRenderable* next_meshes;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->procedural_mesh_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->procedural_mesh_capacity > 0U ? snapshot->procedural_mesh_capacity : 8U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_meshes = (ProceduralMeshRenderable*)realloc(
        snapshot->procedural_meshes,
        next_capacity * sizeof(*next_meshes)
    );
    if (next_meshes == NULL) {
        return false;
    }

    snapshot->procedural_meshes = next_meshes;
    snapshot->procedural_mesh_capacity = next_capacity;
    return true;
}

bool render_world_snapshot_reserve_triangles(RenderWorldSnapshot* snapshot, size_t required_capacity) {
    TriangleRenderable* next_triangles;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->triangle_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->triangle_capacity > 0U ? snapshot->triangle_capacity : 256U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_triangles = (TriangleRenderable*)realloc(
        snapshot->triangles,
        next_capacity * sizeof(*next_triangles)
    );
    if (next_triangles == NULL) {
        return false;
    }

    snapshot->triangles = next_triangles;
    snapshot->triangle_capacity = next_capacity;
    return true;
}

bool render_world_snapshot_reserve_lines(RenderWorldSnapshot* snapshot, size_t required_capacity) {
    LineRenderable* next_lines;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->line_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->line_capacity > 0U ? snapshot->line_capacity : 256U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_lines = (LineRenderable*)realloc(snapshot->lines, next_capacity * sizeof(*next_lines));
    if (next_lines == NULL) {
        return false;
    }

    snapshot->lines = next_lines;
    snapshot->line_capacity = next_capacity;
    return true;
}

bool render_world_snapshot_reserve_particle_surface_mesh_inputs(
    RenderWorldSnapshot* snapshot,
    size_t required_capacity
) {
    ParticleSurfaceMeshBuildInput* next_inputs;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->particle_surface_mesh_input_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->particle_surface_mesh_input_capacity > 0U
        ? snapshot->particle_surface_mesh_input_capacity
        : 4U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_inputs = (ParticleSurfaceMeshBuildInput*)realloc(
        snapshot->particle_surface_mesh_inputs,
        next_capacity * sizeof(*next_inputs)
    );
    if (next_inputs == NULL) {
        return false;
    }

    snapshot->particle_surface_mesh_inputs = next_inputs;
    snapshot->particle_surface_mesh_input_capacity = next_capacity;
    return true;
}

bool render_world_snapshot_reserve_particle_surface_mesh_particles(
    RenderWorldSnapshot* snapshot,
    size_t required_capacity
) {
    PhysicsParticleRenderData* next_particles;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->particle_surface_mesh_particle_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->particle_surface_mesh_particle_capacity > 0U
        ? snapshot->particle_surface_mesh_particle_capacity
        : 256U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_particles = (PhysicsParticleRenderData*)realloc(
        snapshot->particle_surface_mesh_particles,
        next_capacity * sizeof(*next_particles)
    );
    if (next_particles == NULL) {
        return false;
    }

    snapshot->particle_surface_mesh_particles = next_particles;
    snapshot->particle_surface_mesh_particle_capacity = next_capacity;
    return true;
}

bool render_world_snapshot_reserve_debug_collisions(RenderWorldSnapshot* snapshot, size_t required_capacity) {
    DebugCollisionView* next_collisions;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->debug_collision_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->debug_collision_capacity > 0U ? snapshot->debug_collision_capacity : 64U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_collisions = (DebugCollisionView*)realloc(
        snapshot->debug_collisions,
        next_capacity * sizeof(*next_collisions)
    );
    if (next_collisions == NULL) {
        return false;
    }

    snapshot->debug_collisions = next_collisions;
    snapshot->debug_collision_capacity = next_capacity;
    return true;
}

static void render_snapshot_exchange_dispose(RenderSnapshotExchange* exchange) {
    size_t index;

    if (exchange == NULL) {
        return;
    }

    for (index = 0U; index < 3U; ++index) {
        render_snapshot_exchange_free_world(&exchange->buffers[index].world);
    }

    memset(exchange, 0, sizeof(*exchange));
}

RenderSnapshotExchange* render_snapshot_exchange_create(void) {
    RenderSnapshotExchange* exchange = (RenderSnapshotExchange*)calloc(1U, sizeof(*exchange));

    if (exchange == NULL) {
        return NULL;
    }
    if (!render_snapshot_exchange_init(exchange)) {
        free(exchange);
        return NULL;
    }

    return exchange;
}

void render_snapshot_exchange_destroy(RenderSnapshotExchange* exchange) {
    if (exchange == NULL) {
        return;
    }

    render_snapshot_exchange_dispose(exchange);
    free(exchange);
}

void render_world_snapshot_reset(RenderWorldSnapshot* snapshot) {
    if (snapshot == NULL) {
        return;
    }

    snapshot->material_renderable_count = 0U;
    snapshot->procedural_mesh_count = 0U;
    snapshot->triangle_count = 0U;
    snapshot->line_count = 0U;
    snapshot->particle_surface_mesh_input_count = 0U;
    snapshot->particle_surface_mesh_particle_count = 0U;
    snapshot->material_frame_signature = 0U;
    snapshot->material_sort_signature = 0U;
    snapshot->material_instance_signature = 0U;
    snapshot->material_signatures_valid = false;
    snapshot->material_renderables_sorted_by_layer = true;
    snapshot->backdrop_draw = NULL;
    snapshot->backdrop_user_data = NULL;
    snapshot->backdrop_signature = 0U;
    snapshot->metadata_signature = 0U;
    snapshot->metadata_dirty = false;
    snapshot->debug_entity_count = 0U;
    snapshot->debug_collision_count = 0U;
    snapshot->pick_target_count = 0U;
    memset(&snapshot->camera, 0, sizeof(snapshot->camera));
    memset(&snapshot->selected_entity, 0, sizeof(snapshot->selected_entity));
    snapshot->has_selected_entity = false;
    memset(&snapshot->hovered_entity, 0, sizeof(snapshot->hovered_entity));
    snapshot->has_hovered_entity = false;
    snapshot->draw_scene_renderables = false;
    snapshot->draw_debug_world = false;
    snapshot->now_ms = 0U;
}

RenderSnapshotBuffer* render_snapshot_exchange_begin_write(RenderSnapshotExchange* exchange) {
    ENGINE_PROFILE_ZONE_BEGIN(begin_write_zone, "render_snapshot_exchange_begin_write");
    size_t published_index;
    size_t index;
    RenderSnapshotBuffer* buffer;

    if (exchange == NULL) {
        ENGINE_PROFILE_ZONE_END(begin_write_zone);
        return NULL;
    }

    published_index = atomic_load_explicit(&exchange->published_index, memory_order_acquire);
    buffer = NULL;

    for (index = 0U; index < 3U; ++index) {
        size_t candidate = (exchange->write_index + index) % 3U;

        if (candidate == published_index) {
            continue;
        }
        if (atomic_load_explicit(&exchange->reader_counts[candidate], memory_order_acquire) != 0U) {
            continue;
        }

        buffer = &exchange->buffers[candidate];
        exchange->write_index = candidate;
        break;
    }

    if (buffer == NULL) {
        ENGINE_PROFILE_ZONE_END(begin_write_zone);
        return NULL;
    }

    buffer->sequence = 0U;
    buffer->published_at_ms = 0U;
    ENGINE_PROFILE_ZONE_END(begin_write_zone);
    return buffer;
}

void render_snapshot_exchange_publish(RenderSnapshotExchange* exchange, RenderSnapshotBuffer* buffer) {
    ENGINE_PROFILE_ZONE_BEGIN(publish_zone, "render_snapshot_exchange_publish");
    size_t index;

    if (exchange == NULL || buffer == NULL) {
        ENGINE_PROFILE_ZONE_END(publish_zone);
        return;
    }

    for (index = 0U; index < 3U; ++index) {
        if (buffer == &exchange->buffers[index]) {
            break;
        }
    }
    if (index >= 3U) {
        ENGINE_PROFILE_ZONE_END(publish_zone);
        return;
    }

    buffer->sequence = atomic_load_explicit(&exchange->published_sequence, memory_order_relaxed) + 1U;
    buffer->published_at_ms = buffer->world.now_ms;
    atomic_store_explicit(&exchange->published_index, index, memory_order_release);
    atomic_store_explicit(&exchange->published_sequence, buffer->sequence, memory_order_release);
    exchange->write_index = (index + 1U) % 3U;
    ENGINE_PROFILE_ZONE_END(publish_zone);
}

uint64_t render_snapshot_exchange_get_published_sequence(const RenderSnapshotExchange* exchange) {
    if (exchange == NULL) {
        return 0U;
    }

    return atomic_load_explicit(&exchange->published_sequence, memory_order_acquire);
}

const RenderSnapshotBuffer* render_snapshot_exchange_acquire_published(RenderSnapshotExchange* exchange) {
    ENGINE_PROFILE_ZONE_BEGIN(acquire_published_zone, "render_snapshot_exchange_acquire_published");
    uint64_t sequence;

    if (exchange == NULL) {
        ENGINE_PROFILE_ZONE_END(acquire_published_zone);
        return NULL;
    }

    for (;;) {
        size_t index;

        sequence = atomic_load_explicit(&exchange->published_sequence, memory_order_acquire);
        if (sequence == 0U) {
            ENGINE_PROFILE_ZONE_END(acquire_published_zone);
            return NULL;
        }

        index = atomic_load_explicit(&exchange->published_index, memory_order_acquire);
        if (index >= 3U) {
            ENGINE_PROFILE_ZONE_END(acquire_published_zone);
            return NULL;
        }

        atomic_fetch_add_explicit(&exchange->reader_counts[index], 1U, memory_order_acq_rel);
        if (index == atomic_load_explicit(&exchange->published_index, memory_order_acquire)) {
            const RenderSnapshotBuffer* buffer = &exchange->buffers[index];
            ENGINE_PROFILE_ZONE_END(acquire_published_zone);
            return buffer;
        }
        atomic_fetch_sub_explicit(&exchange->reader_counts[index], 1U, memory_order_acq_rel);
    }
}

const RenderSnapshotBuffer* render_snapshot_exchange_acquire_if_new(
    RenderSnapshotExchange* exchange,
    uint64_t last_sequence
) {
    uint64_t sequence;

    if (exchange == NULL) {
        return NULL;
    }

    sequence = atomic_load_explicit(&exchange->published_sequence, memory_order_acquire);
    if (sequence == 0U || sequence == last_sequence) {
        return NULL;
    }

    for (;;) {
        size_t index;

        index = atomic_load_explicit(&exchange->published_index, memory_order_acquire);
        if (index >= 3U) {
            return NULL;
        }

        atomic_fetch_add_explicit(&exchange->reader_counts[index], 1U, memory_order_acq_rel);
        if (index == atomic_load_explicit(&exchange->published_index, memory_order_acquire) &&
            sequence == atomic_load_explicit(&exchange->published_sequence, memory_order_acquire)) {
            return &exchange->buffers[index];
        }
        atomic_fetch_sub_explicit(&exchange->reader_counts[index], 1U, memory_order_acq_rel);

        sequence = atomic_load_explicit(&exchange->published_sequence, memory_order_acquire);
        if (sequence == 0U || sequence == last_sequence) {
            return NULL;
        }
    }
}

void render_snapshot_exchange_release_published(RenderSnapshotExchange* exchange, const RenderSnapshotBuffer* buffer) {
    size_t index;

    if (exchange == NULL || buffer == NULL) {
        return;
    }

    for (index = 0U; index < 3U; ++index) {
        if (buffer == &exchange->buffers[index]) {
            atomic_fetch_sub_explicit(&exchange->reader_counts[index], 1U, memory_order_acq_rel);
            return;
        }
    }
}

void render_world_snapshot_build_frame(const RenderWorldSnapshot* snapshot, RendererFrame* frame) {
    if (snapshot == NULL || frame == NULL) {
        return;
    }

    memset(frame, 0, sizeof(*frame));
    frame->material_renderables = snapshot->material_renderables;
    frame->material_renderable_count = snapshot->material_renderable_count;
    frame->procedural_meshes = snapshot->procedural_meshes;
    frame->procedural_mesh_count = snapshot->procedural_mesh_count;
    frame->triangles = snapshot->triangles;
    frame->triangle_count = snapshot->triangle_count;
    frame->lines = snapshot->lines;
    frame->line_count = snapshot->line_count;
    frame->material_frame_signature = snapshot->material_frame_signature;
    frame->material_sort_signature = snapshot->material_sort_signature;
    frame->material_instance_signature = snapshot->material_instance_signature;
    frame->material_signatures_valid = snapshot->material_signatures_valid;
    frame->material_renderables_sorted_by_layer = snapshot->material_renderables_sorted_by_layer;
    frame->backdrop_draw = snapshot->backdrop_draw;
    frame->backdrop_user_data = snapshot->backdrop_user_data;
    frame->backdrop_signature = snapshot->backdrop_signature;
    frame->metadata_signature = snapshot->metadata_signature;
    frame->metadata_dirty = snapshot->metadata_dirty;
    frame->debug_grid = snapshot->debug_grid;
    frame->debug_entities = snapshot->debug_entities;
    frame->debug_entity_count = snapshot->debug_entity_count;
    frame->debug_collisions = snapshot->debug_collisions;
    frame->debug_collision_count = snapshot->debug_collision_count;
    frame->selected_debug_entity_id = 0U;
    frame->hovered_debug_entity_id = 0U;
    frame->draw_scene_renderables = snapshot->draw_scene_renderables;
    frame->draw_debug = snapshot->draw_debug_world;
    frame->now_ms = snapshot->now_ms;
}

void render_snapshot_buffer_build_frame(const RenderSnapshotBuffer* buffer, RendererFrame* frame) {
    if (buffer == NULL) {
        if (frame != NULL) {
            memset(frame, 0, sizeof(*frame));
        }
        return;
    }

    render_world_snapshot_build_frame(&buffer->world, frame);
}

const RenderStatsSnapshot* render_snapshot_buffer_get_stats(const RenderSnapshotBuffer* buffer) {
    return buffer != NULL ? &buffer->stats : NULL;
}

uint64_t render_snapshot_buffer_get_sequence(const RenderSnapshotBuffer* buffer) {
    return buffer != NULL ? buffer->sequence : 0U;
}

uint64_t render_snapshot_buffer_get_published_at_ms(const RenderSnapshotBuffer* buffer) {
    return buffer != NULL ? buffer->published_at_ms : 0U;
}

uint64_t render_snapshot_buffer_get_now_ms(const RenderSnapshotBuffer* buffer) {
    return buffer != NULL ? buffer->world.now_ms : 0U;
}

const DebugEntityView* render_snapshot_buffer_get_selected_entity(const RenderSnapshotBuffer* buffer) {
    if (buffer == NULL || !buffer->world.has_selected_entity) {
        return NULL;
    }

    return &buffer->world.selected_entity;
}

uint64_t render_snapshot_buffer_pick_entity(
    const RenderSnapshotBuffer* buffer,
    const Camera* camera,
    Vec2 screen_point
) {
    Vec2 world_point;
    size_t index;

    if (buffer == NULL || camera == NULL) {
        return 0U;
    }

    world_point = camera_screen_to_world(camera, screen_point);
    for (index = buffer->world.pick_target_count; index > 0U; --index) {
        const PickTargetView* target = &buffer->world.pick_targets[index - 1U];
        if (target->is_circle) {
            float dx = world_point.x - target->x;
            float dy = world_point.y - target->y;
            if ((dx * dx) + (dy * dy) <= target->radius * target->radius) {
                return target->id;
            }
        } else if (world_point.x >= target->x - target->extent_x &&
                   world_point.x <= target->x + target->extent_x &&
                   world_point.y >= target->y - target->extent_y &&
                   world_point.y <= target->y + target->extent_y) {
            return target->id;
        }
    }

    return 0U;
}

void render_snapshot_buffer_set_sim_timings(
    RenderSnapshotBuffer* buffer,
    float update_ms,
    float sim_ms,
    float input_ms,
    float game_update_ms,
    float fixed_step_wall_ms,
    float drag_ms,
    float snapshot_acquire_ms,
    float snapshot_build_ms
) {
    if (buffer == NULL) {
        return;
    }

    buffer->stats.overlay.update_ms = update_ms;
    buffer->stats.overlay.sim_ms = sim_ms;
    buffer->stats.sim.input_ms = input_ms;
    buffer->stats.sim.game_update_ms = game_update_ms;
    buffer->stats.sim.fixed_step_wall_ms = fixed_step_wall_ms;
    buffer->stats.sim.drag_ms = drag_ms;
    buffer->stats.sim.snapshot_acquire_ms = snapshot_acquire_ms;
    buffer->stats.sim.snapshot_build_ms = snapshot_build_ms;
}
