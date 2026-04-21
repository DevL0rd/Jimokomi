#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTEXCHANGE_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERSNAPSHOTEXCHANGE_H

#include "RenderSnapshot.h"

RenderSnapshotExchange* render_snapshot_exchange_create(void);
void render_snapshot_exchange_destroy(RenderSnapshotExchange* exchange);
RenderSnapshotBuffer* render_snapshot_exchange_begin_write(RenderSnapshotExchange* exchange);
void render_snapshot_exchange_publish(RenderSnapshotExchange* exchange, RenderSnapshotBuffer* buffer);
uint64_t render_snapshot_exchange_get_published_sequence(const RenderSnapshotExchange* exchange);
const RenderSnapshotBuffer* render_snapshot_exchange_acquire_published(RenderSnapshotExchange* exchange);
const RenderSnapshotBuffer* render_snapshot_exchange_acquire_if_new(
    RenderSnapshotExchange* exchange,
    uint64_t last_sequence
);
void render_snapshot_exchange_release_published(RenderSnapshotExchange* exchange, const RenderSnapshotBuffer* buffer);

#endif
