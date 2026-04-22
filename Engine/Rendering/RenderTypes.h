#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERTYPES_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERTYPES_H

#include "Camera.h"
#include "DebugOverlay.h"
#include "RenderCommon.h"
#include "ResourceTypes.h"
#include "Target.h"

typedef struct SpriteRenderable {
    float x;
    float y;
    float angle_radians;
    float anchor_x;
    float anchor_y;
    int layer;
    bool visible;
    ResourceHandle visual_source_handle;
    ResourceHandle material_handle;
    ResourceHandle shader_handle;
    Color32 tint;
    void* user_data;
} SpriteRenderable;

typedef void (*RendererBackdropDrawFn)(Target* target, const Camera* camera, void* user_data);

typedef struct RendererFrame {
    const SpriteRenderable *items;
    size_t item_count;
    uint64_t item_frame_signature;
    uint64_t item_sort_signature;
    uint64_t item_instance_signature;
    bool item_signatures_valid;
    bool items_sorted_by_layer;
    RendererBackdropDrawFn backdrop_draw;
    void* backdrop_user_data;
    uint64_t backdrop_signature;
    uint64_t metadata_signature;
    bool metadata_dirty;
    DebugGridView debug_grid;
    DebugEntityView *debug_entities;
    size_t debug_entity_count;
    DebugCollisionView *debug_collisions;
    size_t debug_collision_count;
    uint64_t selected_debug_entity_id;
    uint64_t hovered_debug_entity_id;
    bool draw_sprites;
    bool draw_debug;
    uint64_t now_ms;
} RendererFrame;

#endif
