#include "RenderSnapshot.h"
#include "RenderSnapshotExchange.h"
#include "RenderSnapshotInternal.h"

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

    free(world->items);
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

bool render_world_snapshot_reserve_items(RenderWorldSnapshot* snapshot, size_t required_capacity) {
    SpriteRenderable* next_items;
    DebugEntityView* next_debug_entities;
    PickTargetView* next_pick_targets;
    size_t next_capacity;

    if (snapshot == NULL) {
        return false;
    }
    if (snapshot->item_capacity >= required_capacity &&
        snapshot->debug_entity_capacity >= required_capacity &&
        snapshot->pick_target_capacity >= required_capacity) {
        return true;
    }

    next_capacity = snapshot->item_capacity > 0U ? snapshot->item_capacity : 64U;
    while (next_capacity < required_capacity) {
        next_capacity *= 2U;
    }

    next_items = (SpriteRenderable*)realloc(snapshot->items, next_capacity * sizeof(*next_items));
    if (next_items == NULL) {
        return false;
    }
    snapshot->items = next_items;
    snapshot->item_capacity = next_capacity;

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

    snapshot->item_count = 0U;
    snapshot->item_frame_signature = 0U;
    snapshot->item_sort_signature = 0U;
    snapshot->item_instance_signature = 0U;
    snapshot->item_signatures_valid = false;
    snapshot->items_sorted_by_layer = true;
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
    snapshot->draw_sprites = false;
    snapshot->draw_debug_world = false;
    snapshot->now_ms = 0U;
}

RenderSnapshotBuffer* render_snapshot_exchange_begin_write(RenderSnapshotExchange* exchange) {
    size_t published_index;
    size_t index;
    RenderSnapshotBuffer* buffer;

    if (exchange == NULL) {
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
        return NULL;
    }

    buffer->sequence = 0U;
    buffer->published_at_ms = 0U;
    return buffer;
}

void render_snapshot_exchange_publish(RenderSnapshotExchange* exchange, RenderSnapshotBuffer* buffer) {
    size_t index;

    if (exchange == NULL || buffer == NULL) {
        return;
    }

    for (index = 0U; index < 3U; ++index) {
        if (buffer == &exchange->buffers[index]) {
            break;
        }
    }
    if (index >= 3U) {
        return;
    }

    buffer->sequence = atomic_load_explicit(&exchange->published_sequence, memory_order_relaxed) + 1U;
    buffer->published_at_ms = buffer->world.now_ms;
    atomic_store_explicit(&exchange->published_index, index, memory_order_release);
    atomic_store_explicit(&exchange->published_sequence, buffer->sequence, memory_order_release);
    exchange->write_index = (index + 1U) % 3U;
}

uint64_t render_snapshot_exchange_get_published_sequence(const RenderSnapshotExchange* exchange) {
    if (exchange == NULL) {
        return 0U;
    }

    return atomic_load_explicit(&exchange->published_sequence, memory_order_acquire);
}

const RenderSnapshotBuffer* render_snapshot_exchange_acquire_published(RenderSnapshotExchange* exchange) {
    uint64_t sequence;

    if (exchange == NULL) {
        return NULL;
    }

    for (;;) {
        size_t index;

        sequence = atomic_load_explicit(&exchange->published_sequence, memory_order_acquire);
        if (sequence == 0U) {
            return NULL;
        }

        index = atomic_load_explicit(&exchange->published_index, memory_order_acquire);
        if (index >= 3U) {
            return NULL;
        }

        atomic_fetch_add_explicit(&exchange->reader_counts[index], 1U, memory_order_acq_rel);
        if (index == atomic_load_explicit(&exchange->published_index, memory_order_acquire)) {
            return &exchange->buffers[index];
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
    frame->items = snapshot->items;
    frame->item_count = snapshot->item_count;
    frame->item_frame_signature = snapshot->item_frame_signature;
    frame->item_sort_signature = snapshot->item_sort_signature;
    frame->item_instance_signature = snapshot->item_instance_signature;
    frame->item_signatures_valid = snapshot->item_signatures_valid;
    frame->items_sorted_by_layer = snapshot->items_sorted_by_layer;
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
    frame->draw_sprites = snapshot->draw_sprites;
    frame->draw_debug = snapshot->draw_debug_world;
    frame->now_ms = snapshot->now_ms;
}
