#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERCOMMON_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERCOMMON_H

#include "../Core/Geometry.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Color32 {
    uint32_t value;
} Color32;

static inline Color32 color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    Color32 color;
    color.value = ((uint32_t)a << 24U) | ((uint32_t)r << 16U) | ((uint32_t)g << 8U) | (uint32_t)b;
    return color;
}

typedef struct Surface {
    int width;
    int height;
    void *pixels;
    size_t bytes_per_pixel;
    uint64_t generation;
} Surface;

static inline char *string_duplicate(const char *text) {
    size_t length;
    char *copy;

    if (text == NULL) {
        return NULL;
    }

    length = strlen(text) + 1U;
    copy = (char *)malloc(length);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length);
    return copy;
}

static inline void safe_free(void **pointer) {
    if (pointer != NULL && *pointer != NULL) {
        free(*pointer);
        *pointer = NULL;
    }
}

#endif
