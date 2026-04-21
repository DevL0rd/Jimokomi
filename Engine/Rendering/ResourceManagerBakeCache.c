#include "ResourceManagerInternal.h"

#include <stdlib.h>
#include <string.h>

bool resource_manager_reserve_baked_surfaces(
    ResourceManager* manager,
    size_t required
) {
    BakedSurfaceResource* next_values;
    size_t next_capacity;

    if (manager->baked_surface_capacity >= required) {
        return true;
    }

    next_capacity = manager->baked_surface_capacity > 0U ? manager->baked_surface_capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedSurfaceResource*)realloc(
        manager->baked_surfaces,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->baked_surfaces = next_values;
    manager->baked_surface_capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_surface_slots(
    ResourceManager* manager,
    size_t required
) {
    BakedSurfaceSlot* next_slots;
    size_t next_capacity;
    size_t index;

    if (manager->baked_surface_slot_capacity >= required) {
        return true;
    }

    next_capacity = manager->baked_surface_slot_capacity > 0U ? manager->baked_surface_slot_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_slots = (BakedSurfaceSlot*)calloc(next_capacity, sizeof(*next_slots));
    if (next_slots == NULL) {
        return false;
    }

    for (index = 0U; index < manager->baked_surface_slot_capacity; ++index) {
        BakedSurfaceSlot* slot = &manager->baked_surface_slots[index];
        if (slot->state != SLOT_STATE_FILLED) {
            continue;
        }

        {
            size_t slot_index = (size_t)(resource_manager_hash_baked_key(slot->key) & (uint64_t)(next_capacity - 1U));
            while (next_slots[slot_index].state == SLOT_STATE_FILLED) {
                slot_index = (slot_index + 1U) & (next_capacity - 1U);
            }
            next_slots[slot_index] = *slot;
        }
    }

    free(manager->baked_surface_slots);
    manager->baked_surface_slots = next_slots;
    manager->baked_surface_slot_capacity = next_capacity;
    return true;
}

bool resource_manager_insert_baked_surface_slot(
    ResourceManager* manager,
    BakedSurfaceKey key,
    size_t value_index
) {
    size_t slot_index;

    if (manager == NULL) {
        return false;
    }

    if ((manager->baked_surface_slot_count + 1U) * 10U >= manager->baked_surface_slot_capacity * 7U) {
        if (!resource_manager_reserve_baked_surface_slots(
                manager,
                manager->baked_surface_slot_capacity > 0U
                    ? manager->baked_surface_slot_capacity * 2U
                    : 64U)) {
            return false;
        }
    }

    if (manager->baked_surface_slot_capacity == 0U &&
        !resource_manager_reserve_baked_surface_slots(manager, 64U)) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->baked_surface_slot_capacity - 1U));
    while (manager->baked_surface_slots[slot_index].state == SLOT_STATE_FILLED) {
        slot_index = (slot_index + 1U) & (manager->baked_surface_slot_capacity - 1U);
    }

    manager->baked_surface_slots[slot_index].state = SLOT_STATE_FILLED;
    manager->baked_surface_slots[slot_index].key = key;
    manager->baked_surface_slots[slot_index].index = value_index;
    manager->baked_surface_slot_count += 1U;
    return true;
}

const Surface* resource_manager_find_baked_surface(
    const ResourceManager* manager,
    BakedSurfaceKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->baked_surface_slot_capacity == 0U) {
        return NULL;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->baked_surface_slot_capacity - 1U));
    while (manager->baked_surface_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->baked_surface_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->baked_surface_slots[slot_index].key, key)) {
            return manager->baked_surfaces[manager->baked_surface_slots[slot_index].index].surface;
        }
        slot_index = (slot_index + 1U) & (manager->baked_surface_slot_capacity - 1U);
    }

    return NULL;
}

const Surface* resource_manager_get_baked_surface(
    ResourceManager* manager,
    ResourceHandle visual_source_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedSurfacePass pass,
    void* user_data
) {
    const VisualSourceResource* source;
    const MaterialResource* material;
    const Surface* baked_surface;
    BakedSurfaceKey key;

    if (manager == NULL || manager->backend == NULL ||
        manager->backend->create_surface == NULL ||
        manager->backend->set_target == NULL ||
        manager->backend->reset_target == NULL ||
        manager->backend->clear == NULL) {
        return NULL;
    }

    source = resource_manager_get_visual_source(manager, visual_source_handle);
    material = resource_manager_get_material(manager, material_handle);
    if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return NULL;
    }

    key.visual_source_id = visual_source_handle.id;
    key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
    key.shader_id = shader_handle.id;
    key.frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.pass = (uint8_t)pass;

    baked_surface = resource_manager_find_baked_surface(manager, key);
    if (baked_surface != NULL) {
        manager->bake_cache_hits += 1U;
        return baked_surface;
    }

    manager->bake_cache_misses += 1U;
    (void)user_data;
    return NULL;
}

const Surface* resource_manager_build_baked_surface(
    ResourceManager* manager,
    const VisualSourceResource* source,
    const MaterialResource* material,
    const ShaderResource* shader,
    BakedSurfaceKey key,
    void* user_data
) {
    size_t index;
    Surface* surface;
    Target target;
    ProceduralTextureContext context;
    Material neutral_material;
    const Material* context_material;

    if (manager == NULL || source == NULL || manager->backend == NULL) {
        return NULL;
    }

    if (!resource_manager_reserve_baked_surfaces(manager, manager->baked_surface_count + 1U)) {
        return NULL;
    }

    surface = manager->backend->create_surface(manager->backend->userdata, source->width, source->height);
    if (surface == NULL) {
        return NULL;
    }

    manager->bake_requests_this_frame += 1U;
    manager->backend->set_target(manager->backend->userdata, surface);
    manager->backend->clear(manager->backend->userdata, color_rgba(0U, 0U, 0U, 0U));
    target_init(&target, manager->backend, 0.0f, 0.0f);

    memset(&context, 0, sizeof(context));
    context.now_ms = (uint64_t)(((source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps) > 0.0f)
        ? ((double)key.frame_index / (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps)) * 1000.0
        : 0.0);
    context.time_seconds = (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps) > 0.0f
        ? (float)key.frame_index / (source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps)
        : 0.0f;
    context.animation_fps = source->bake_animation_fps > 0.0f ? source->bake_animation_fps : source->animation_fps;
    context.frame_index = key.frame_index;
    context.angle_radians = 0.0f;
    memset(&neutral_material, 0, sizeof(neutral_material));
    neutral_material.base_color = 0xffffffU;
    neutral_material.accent_color = 0xd0d8e8U;
    neutral_material.glow_color = 0xffffffU;
    neutral_material.glare_color = 0xffffffU;
    neutral_material.emissive = 1.0f;
    neutral_material.distortion = 0.0f;
    neutral_material.glare_strength = 1.0f;
    neutral_material.pulse_rate = 1.0f;
    neutral_material.shader_style = shader != NULL
        ? shader->style
        : (material != NULL ? material->value.shader_style : SHADER_STYLE_NONE);
    context_material = source->bake_ignores_material
        ? NULL
        : (material != NULL ? &material->value : &neutral_material);
    context.material = context_material;
    context.shader_style = shader != NULL
        ? shader->style
        : (material != NULL ? material->value.shader_style : neutral_material.shader_style);

    if (key.pass == BAKED_SURFACE_PASS_BODY) {
        source->draw_body(&target, &context, user_data);
    } else {
        source->draw_overlay(&target, &context, user_data);
    }

    manager->backend->reset_target(manager->backend->userdata);

    index = manager->baked_surface_count++;
    manager->baked_surfaces[index].key = key;
    manager->baked_surfaces[index].surface = surface;
    if (!resource_manager_insert_baked_surface_slot(manager, key, index)) {
        manager->backend->destroy_surface(manager->backend->userdata, surface);
        manager->baked_surface_count -= 1U;
        return NULL;
    }
    resource_manager_mark_dirty_baked_surface(manager, key);
    return surface;
}
