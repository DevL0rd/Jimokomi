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

static inline Color32 color_rgb_from_hsv(float hue, float saturation, float value) {
    float r;
    float g;
    float b;
    float scaled_hue;
    float sector_fraction;
    float p;
    float q;
    float t;
    int sector;

    while (hue < 0.0f) {
        hue += 360.0f;
    }
    while (hue >= 360.0f) {
        hue -= 360.0f;
    }

    if (saturation <= 0.0f) {
        r = value;
        g = value;
        b = value;
    } else {
        scaled_hue = hue / 60.0f;
        sector = (int)scaled_hue;
        sector_fraction = scaled_hue - (float)sector;
        p = value * (1.0f - saturation);
        q = value * (1.0f - saturation * sector_fraction);
        t = value * (1.0f - saturation * (1.0f - sector_fraction));

        switch (sector) {
            case 0: r = value; g = t; b = p; break;
            case 1: r = q; g = value; b = p; break;
            case 2: r = p; g = value; b = t; break;
            case 3: r = p; g = q; b = value; break;
            case 4: r = t; g = p; b = value; break;
            default: r = value; g = p; b = q; break;
        }
    }

    return color_rgba(
        (uint8_t)(clamp_f(r, 0.0f, 1.0f) * 255.0f),
        (uint8_t)(clamp_f(g, 0.0f, 1.0f) * 255.0f),
        (uint8_t)(clamp_f(b, 0.0f, 1.0f) * 255.0f),
        0U
    );
}

typedef struct Texture {
    int width;
    int height;
    void *pixels;
    size_t bytes_per_pixel;
    uint64_t generation;
} Texture;

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
