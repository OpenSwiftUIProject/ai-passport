#pragma once
#include <stdint.h>
// Host preview has no ESP32 heap measurement.
static inline uint32_t esp_get_free_heap_size(void) { return 0; }
