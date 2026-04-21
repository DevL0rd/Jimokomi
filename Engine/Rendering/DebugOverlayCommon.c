#include "DebugOverlayInternal.h"

#include "../Core/PlatformRuntimeInternal.h"

#include <math.h>

double debug_overlay_now_ms(void) {
    return engine_platform_now_ms();
}

uint64_t debug_hash_u64(uint64_t hash, uint64_t value) {
    hash ^= value;
    hash *= 1099511628211ULL;
    return hash;
}

uint64_t debug_hash_i32(uint64_t hash, int32_t value) {
    return debug_hash_u64(hash, (uint32_t)value);
}

int32_t debug_round_tenths(float value) {
    return (int32_t)lrintf(value * 10.0f);
}

int32_t debug_round_whole(float value) {
    return (int32_t)lrintf(value);
}

int32_t debug_round_quarters(float value) {
    return (int32_t)lrintf(value * 4.0f);
}
