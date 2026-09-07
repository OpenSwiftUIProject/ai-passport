#pragma once
#include <stdint.h>
// Explicit host fixture. Device page entry uses ESP-IDF's RNG.
static inline uint32_t esp_random(void) { return 0x2048; }
