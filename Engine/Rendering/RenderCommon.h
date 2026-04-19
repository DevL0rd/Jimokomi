#ifndef JIMOKOMI_ENGINE_RENDERING_RENDERCOMMON_H
#define JIMOKOMI_ENGINE_RENDERING_RENDERCOMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct Vec2 {
    float x;
    float y;
} Vec2;

typedef struct Rect {
    float x;
    float y;
    float w;
    float h;
} Rect;

typedef struct Aabb {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
} Aabb;

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

static inline float clamp_f(float value, float minimum, float maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static inline int clamp_i(int value, int minimum, int maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static inline float lerp_f(float from, float to, float alpha) {
    return from + (to - from) * clamp_f(alpha, 0.0f, 1.0f);
}

static inline float inverse_lerp_f(float from, float to, float value) {
    float range = to - from;
    if (fabsf(range) <= 0.00001f) {
        return 0.0f;
    }
    return clamp_f((value - from) / range, 0.0f, 1.0f);
}

static inline float approach_f(float current, float target, float delta) {
    if (current < target) {
        return current + delta > target ? target : current + delta;
    }
    return current - delta < target ? target : current - delta;
}

static inline float exp_smoothing_alpha(float dt_seconds, float smoothing_seconds) {
    if (smoothing_seconds <= 0.0f) {
        return 1.0f;
    }
    return 1.0f - expf(-dt_seconds / smoothing_seconds);
}

static inline float damp_f(float current, float target, float smoothing_seconds, float dt_seconds) {
    return lerp_f(current, target, exp_smoothing_alpha(dt_seconds, smoothing_seconds));
}

static inline Vec2 vec2_lerp(Vec2 from, Vec2 to, float alpha) {
    Vec2 result;
    result.x = lerp_f(from.x, to.x, alpha);
    result.y = lerp_f(from.y, to.y, alpha);
    return result;
}

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
