#ifndef JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_QUEUE_INTERNAL_H
#define JIMOKOMI_ENGINE_RENDERING_RESOURCEMANAGER_BAKE_QUEUE_INTERNAL_H

#include "ResourceManagerInternal.h"

typedef struct PendingBakeRequest {
    BakedSurfaceKey key;
    uint32_t priority;
} PendingBakeRequest;

typedef struct PendingBakeSlot {
    uint8_t state;
    BakedSurfaceKey key;
} PendingBakeSlot;

typedef struct BakeInterestEntry {
    BakedSurfaceKey key;
    uint32_t total_hits;
    uint32_t frame_hits;
    uint64_t last_seen_frame;
} BakeInterestEntry;

struct ResourceBakeQueueState {
    PendingBakeRequest* static_pending_bake_requests;
    size_t static_pending_bake_request_count;
    size_t static_pending_bake_request_capacity;
    size_t static_pending_bake_request_head;
    PendingBakeRequest* transient_pending_bake_requests;
    size_t transient_pending_bake_request_count;
    size_t transient_pending_bake_request_capacity;
    size_t transient_pending_bake_request_head;
    PendingBakeSlot* pending_bake_slots;
    size_t pending_bake_slot_capacity;
    size_t pending_bake_slot_count;
    BakeInterestEntry* bake_interest_entries;
    size_t bake_interest_count;
    size_t bake_interest_capacity;
    size_t bake_requests_this_frame;
    double bake_time_budget_ms;
    double last_bake_process_ms;
    size_t bake_admission_total_hits;
    size_t bake_admission_frame_hits;
    uint64_t frame_serial;
    uint32_t* visual_source_last_requested_frame_indices;
    size_t visual_source_last_requested_frame_capacity;
};

#endif
