#include "ResourceManagerBakeCacheInternal.h"
#include "ResourceManagerInvalidationInternal.h"
#include "ResourceManagerStatsInternal.h"
#include "GeneratedFrame.h"

#include "../Core/Memory.h"

#include <stdlib.h>
#include <string.h>

bool resource_manager_reserve_baked_textures(
    ResourceManager* manager,
    size_t required
) {
    BakedTextureResource* next_values;
    size_t next_capacity;

    if (manager->bake_cache->baked_texture_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_cache->baked_texture_capacity > 0U ? manager->bake_cache->baked_texture_capacity : 16U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedTextureResource*)realloc(
        manager->bake_cache->baked_textures,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->bake_cache->baked_textures = next_values;
    manager->bake_cache->baked_texture_capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_meshes(
    ResourceManager* manager,
    size_t required
) {
    BakedMeshResource* next_values;
    size_t next_capacity;

    if (manager->bake_cache->baked_mesh_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_cache->baked_mesh_capacity > 0U ? manager->bake_cache->baked_mesh_capacity : 8U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_values = (BakedMeshResource*)realloc(
        manager->bake_cache->baked_meshes,
        next_capacity * sizeof(*next_values)
    );
    if (next_values == NULL) {
        return false;
    }

    manager->bake_cache->baked_meshes = next_values;
    manager->bake_cache->baked_mesh_capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_mesh_triangles(
    Mesh* mesh,
    size_t required
) {
    TriangleRenderable* next_triangles;
    size_t next_capacity;

    if (mesh->triangle_capacity >= required) {
        return true;
    }

    next_capacity = mesh->triangle_capacity > 0U ? mesh->triangle_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_triangles = (TriangleRenderable*)realloc(
        mesh->triangles,
        next_capacity * sizeof(*next_triangles)
    );
    if (next_triangles == NULL) {
        return false;
    }

    mesh->triangles = next_triangles;
    mesh->triangle_capacity = next_capacity;
    return true;
}

static bool resource_manager_reserve_baked_mesh_lines(
    Mesh* mesh,
    size_t required
) {
    LineRenderable* next_lines;
    size_t next_capacity;

    if (mesh->line_capacity >= required) {
        return true;
    }

    next_capacity = mesh->line_capacity > 0U ? mesh->line_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_lines = (LineRenderable*)realloc(
        mesh->lines,
        next_capacity * sizeof(*next_lines)
    );
    if (next_lines == NULL) {
        return false;
    }

    mesh->lines = next_lines;
    mesh->line_capacity = next_capacity;
    return true;
}

static bool resource_manager_baked_mesh_add_triangle(
    void* user_data,
    Vec2 a,
    Vec2 b,
    Vec2 c,
    Color32 color,
    int layer
) {
    Mesh* mesh = (Mesh*)user_data;
    TriangleRenderable* triangle;

    if (mesh == NULL ||
        !resource_manager_reserve_baked_mesh_triangles(mesh, mesh->triangle_count + 1U)) {
        return false;
    }

    triangle = &mesh->triangles[mesh->triangle_count++];
    triangle->a = a;
    triangle->b = b;
    triangle->c = c;
    triangle->uv_a = a;
    triangle->uv_b = b;
    triangle->uv_c = c;
    triangle->color = color;
    triangle->layer = layer;
    triangle->visible = true;
    triangle->screen_space_valid = false;
    return true;
}

static bool resource_manager_baked_mesh_add_line(
    void* user_data,
    Vec2 a,
    Vec2 b,
    Color32 color,
    int layer
) {
    Mesh* mesh = (Mesh*)user_data;
    LineRenderable* line;

    if (mesh == NULL ||
        !resource_manager_reserve_baked_mesh_lines(mesh, mesh->line_count + 1U)) {
        return false;
    }

    line = &mesh->lines[mesh->line_count++];
    line->a = a;
    line->b = b;
    line->color = color;
    line->layer = layer;
    line->visible = true;
    return true;
}

static bool resource_manager_reserve_baked_texture_slots(
    ResourceManager* manager,
    size_t required
) {
    BakedTextureSlot* next_slots;
    size_t next_capacity;
    size_t index;

    if (manager->bake_cache->baked_texture_slot_capacity >= required) {
        return true;
    }

    next_capacity = manager->bake_cache->baked_texture_slot_capacity > 0U ? manager->bake_cache->baked_texture_slot_capacity : 64U;
    while (next_capacity < required) {
        next_capacity *= 2U;
    }

    next_slots = (BakedTextureSlot*)calloc(next_capacity, sizeof(*next_slots));
    if (next_slots == NULL) {
        return false;
    }

    for (index = 0U; index < manager->bake_cache->baked_texture_slot_capacity; ++index) {
        BakedTextureSlot* slot = &manager->bake_cache->baked_texture_slots[index];
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

    free(manager->bake_cache->baked_texture_slots);
    manager->bake_cache->baked_texture_slots = next_slots;
    manager->bake_cache->baked_texture_slot_capacity = next_capacity;
    return true;
}

bool resource_manager_insert_baked_texture_slot(
    ResourceManager* manager,
    BakedTextureKey key,
    size_t value_index
) {
    size_t slot_index;

    if (manager == NULL) {
        return false;
    }

    if ((manager->bake_cache->baked_texture_slot_count + 1U) * 10U >= manager->bake_cache->baked_texture_slot_capacity * 7U) {
        if (!resource_manager_reserve_baked_texture_slots(
                manager,
                manager->bake_cache->baked_texture_slot_capacity > 0U
                    ? manager->bake_cache->baked_texture_slot_capacity * 2U
                    : 64U)) {
            return false;
        }
    }

    if (manager->bake_cache->baked_texture_slot_capacity == 0U &&
        !resource_manager_reserve_baked_texture_slots(manager, 64U)) {
        return false;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_cache->baked_texture_slot_capacity - 1U));
    while (manager->bake_cache->baked_texture_slots[slot_index].state == SLOT_STATE_FILLED) {
        slot_index = (slot_index + 1U) & (manager->bake_cache->baked_texture_slot_capacity - 1U);
    }

    manager->bake_cache->baked_texture_slots[slot_index].state = SLOT_STATE_FILLED;
    manager->bake_cache->baked_texture_slots[slot_index].key = key;
    manager->bake_cache->baked_texture_slots[slot_index].index = value_index;
    manager->bake_cache->baked_texture_slot_count += 1U;
    return true;
}

const Texture* resource_manager_find_baked_texture(
    const ResourceManager* manager,
    BakedTextureKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->bake_cache->baked_texture_slot_capacity == 0U) {
        return NULL;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_cache->baked_texture_slot_capacity - 1U));
    while (manager->bake_cache->baked_texture_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->bake_cache->baked_texture_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->bake_cache->baked_texture_slots[slot_index].key, key)) {
            return manager->bake_cache->baked_textures[manager->bake_cache->baked_texture_slots[slot_index].index].texture;
        }
        slot_index = (slot_index + 1U) & (manager->bake_cache->baked_texture_slot_capacity - 1U);
    }

    return NULL;
}

Texture* resource_manager_find_mutable_baked_texture(
    ResourceManager* manager,
    BakedTextureKey key
) {
    size_t slot_index;

    if (manager == NULL || manager->bake_cache->baked_texture_slot_capacity == 0U) {
        return NULL;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_cache->baked_texture_slot_capacity - 1U));
    while (manager->bake_cache->baked_texture_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->bake_cache->baked_texture_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->bake_cache->baked_texture_slots[slot_index].key, key)) {
            return manager->bake_cache->baked_textures[manager->bake_cache->baked_texture_slots[slot_index].index].texture;
        }
        slot_index = (slot_index + 1U) & (manager->bake_cache->baked_texture_slot_capacity - 1U);
    }

    return NULL;
}

const Texture* resource_manager_find_baked_texture_for_source_frame(
    const ResourceManager* manager,
    BakedTextureKey key,
    uint32_t source_frame_index
) {
    size_t slot_index;

    if (manager == NULL || manager->bake_cache->baked_texture_slot_capacity == 0U) {
        return NULL;
    }

    slot_index = (size_t)(resource_manager_hash_baked_key(key) &
        (uint64_t)(manager->bake_cache->baked_texture_slot_capacity - 1U));
    while (manager->bake_cache->baked_texture_slots[slot_index].state != SLOT_STATE_EMPTY) {
        if (manager->bake_cache->baked_texture_slots[slot_index].state == SLOT_STATE_FILLED &&
            resource_manager_baked_key_equals(manager->bake_cache->baked_texture_slots[slot_index].key, key)) {
            const BakedTextureResource* resource =
                &manager->bake_cache->baked_textures[manager->bake_cache->baked_texture_slots[slot_index].index];
            return resource->source_frame_index == source_frame_index ? resource->texture : NULL;
        }
        slot_index = (slot_index + 1U) & (manager->bake_cache->baked_texture_slot_capacity - 1U);
    }

    return NULL;
}

bool resource_manager_store_baked_texture(
    ResourceManager* manager,
    BakedTextureKey key,
    Texture* texture,
    uint32_t source_frame_index
) {
    size_t index;
    size_t slot_index;

    if (manager == NULL || texture == NULL) {
        return false;
    }

    if (manager->bake_cache->baked_texture_slot_capacity > 0U) {
        slot_index = (size_t)(resource_manager_hash_baked_key(key) &
            (uint64_t)(manager->bake_cache->baked_texture_slot_capacity - 1U));
        while (manager->bake_cache->baked_texture_slots[slot_index].state != SLOT_STATE_EMPTY) {
            if (manager->bake_cache->baked_texture_slots[slot_index].state == SLOT_STATE_FILLED &&
                resource_manager_baked_key_equals(manager->bake_cache->baked_texture_slots[slot_index].key, key)) {
                index = manager->bake_cache->baked_texture_slots[slot_index].index;
                if (manager->backend != NULL && manager->backend->destroy_texture != NULL &&
                    manager->bake_cache->baked_textures[index].texture != NULL &&
                    manager->bake_cache->baked_textures[index].texture != texture) {
                    manager->backend->destroy_texture(
                        manager->backend->userdata,
                        manager->bake_cache->baked_textures[index].texture
                    );
                }
                manager->bake_cache->baked_textures[index].texture = texture;
                manager->bake_cache->baked_textures[index].source_frame_index = source_frame_index;
                resource_manager_mark_dirty_baked_texture(manager, key);
                return true;
            }
            slot_index = (slot_index + 1U) & (manager->bake_cache->baked_texture_slot_capacity - 1U);
        }
    }

    if (!resource_manager_reserve_baked_textures(manager, manager->bake_cache->baked_texture_count + 1U)) {
        return false;
    }

    index = manager->bake_cache->baked_texture_count++;
    manager->bake_cache->baked_textures[index].key = key;
    manager->bake_cache->baked_textures[index].texture = texture;
    manager->bake_cache->baked_textures[index].source_frame_index = source_frame_index;
    if (!resource_manager_insert_baked_texture_slot(manager, key, index)) {
        manager->bake_cache->baked_texture_count -= 1U;
        manager->bake_cache->baked_textures[index].texture = NULL;
        return false;
    }

    resource_manager_mark_dirty_baked_texture(manager, key);
    return true;
}

const Texture* resource_manager_get_baked_texture(
    ResourceManager* manager,
    ResourceHandle procedural_texture_handle,
    ResourceHandle material_handle,
    ResourceHandle shader_handle,
    uint32_t frame_index,
    BakedTexturePass pass,
    void* user_data
) {
    const ProceduralTextureResource* source;
    const MaterialResource* material;
    const Texture* baked_texture;
    BakedTextureKey key;

    if (manager == NULL || manager->backend == NULL ||
        manager->backend->create_texture == NULL ||
        manager->backend->set_target == NULL ||
        manager->backend->reset_target == NULL ||
        manager->backend->clear == NULL) {
        return NULL;
    }

    source = resource_manager_get_procedural_texture(manager, procedural_texture_handle);
    material = resource_manager_get_material(manager, material_handle);
    if (source == NULL || material == NULL || !resource_manager_is_bake_eligible(source, pass)) {
        return NULL;
    }

    key.procedural_texture_id = procedural_texture_handle.id;
    key.material_id = source->bake_ignores_material ? 0U : material_handle.id;
    key.shader_id = shader_handle.id;
    key.frame_index = resource_manager_normalize_frame_index(source, frame_index);
    key.pass = (uint8_t)pass;

    baked_texture = source->frames.cache_policy == BAKE_POLICY_REFRESH_FRAME
        ? resource_manager_find_baked_texture_for_source_frame(
            manager,
            key,
            generated_frame_bake_source_index(&source->frames, frame_index)
        )
        : resource_manager_find_baked_texture(manager, key);
    if (baked_texture != NULL) {
        manager->stats->bake_cache_hits += 1U;
        return baked_texture;
    }

    manager->stats->bake_cache_misses += 1U;
    (void)user_data;
    return NULL;
}

const Mesh* resource_manager_get_or_create_baked_mesh(
    ResourceManager* manager,
    ResourceHandle procedural_mesh_handle,
    uint64_t instance_signature,
    uint32_t frame_index,
    void* user_data
) {
    const ProceduralMeshResource* source;
    BakedMeshResource* mesh;
    ProceduralMeshWriter writer;
    ProceduralMeshContext context;
    uint32_t cache_frame_index;
    uint32_t source_frame_index;
    size_t index;

    if (manager == NULL || procedural_mesh_handle.id == 0U) {
        return NULL;
    }

    source = resource_manager_get_procedural_mesh(manager, procedural_mesh_handle);
    if (source == NULL || source->build_mesh == NULL ||
        source->frames.cache_policy == BAKE_POLICY_DISABLED ||
        !source->frames.instance_invariant) {
        return NULL;
    }

    cache_frame_index = generated_frame_cache_index(&source->frames, frame_index);
    source_frame_index = generated_frame_bake_source_index(&source->frames, frame_index);
    for (index = 0U; index < manager->bake_cache->baked_mesh_count; ++index) {
        mesh = &manager->bake_cache->baked_meshes[index];
        if (mesh->procedural_mesh_handle.id == procedural_mesh_handle.id &&
            mesh->instance_signature == instance_signature &&
            mesh->frame_index == cache_frame_index &&
            (source->frames.cache_policy != BAKE_POLICY_REFRESH_FRAME ||
             mesh->source_frame_index == source_frame_index)) {
            return mesh->mesh;
        }
    }

    for (index = 0U; index < manager->bake_cache->baked_mesh_count; ++index) {
        mesh = &manager->bake_cache->baked_meshes[index];
        if (mesh->procedural_mesh_handle.id == procedural_mesh_handle.id &&
            mesh->instance_signature == instance_signature &&
            mesh->frame_index == cache_frame_index) {
            if (mesh->mesh != NULL) {
                mesh->mesh->triangle_count = 0U;
                mesh->mesh->line_count = 0U;
            }
            mesh->source_frame_index = source_frame_index;
            goto build_mesh;
        }
    }

    if (!resource_manager_reserve_baked_meshes(manager, manager->bake_cache->baked_mesh_count + 1U)) {
        return NULL;
    }

    mesh = &manager->bake_cache->baked_meshes[manager->bake_cache->baked_mesh_count++];
    memset(mesh, 0, sizeof(*mesh));
    mesh->procedural_mesh_handle = procedural_mesh_handle;
    mesh->instance_signature = instance_signature;
    mesh->frame_index = cache_frame_index;
    mesh->source_frame_index = source_frame_index;
    mesh->mesh = (Mesh*)calloc(1U, sizeof(*mesh->mesh));
    if (mesh->mesh == NULL) {
        manager->bake_cache->baked_mesh_count -= 1U;
        return NULL;
    }

build_mesh:
    if (mesh->mesh == NULL) {
        return NULL;
    }
    memset(&writer, 0, sizeof(writer));
    writer.user_data = mesh->mesh;
    writer.add_triangle = resource_manager_baked_mesh_add_triangle;
    writer.add_line = resource_manager_baked_mesh_add_line;

    memset(&context, 0, sizeof(context));
    context.time_seconds = generated_frame_cache_time_seconds(&source->frames, source_frame_index);
    context.now_ms = (uint64_t)((double)context.time_seconds * 1000.0);
    context.animation_fps = generated_frame_cache_fps(&source->frames);
    context.frame_index = source_frame_index;

    if (!source->build_mesh(&writer, &context, user_data)) {
        mesh->mesh->triangle_count = 0U;
        mesh->mesh->line_count = 0U;
        return NULL;
    }

    return mesh->mesh;
}
