#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Hardware-independent, bounded wire format shared with host golden tests.
bool screen_request_id(const char *line, uint32_t *id);
uint32_t screen_crc32(const uint8_t *data, size_t length);
size_t screen_encode_row(char *output, size_t capacity, uint32_t id,
                         uint16_t x, uint16_t y, uint16_t width,
                         const uint8_t *rgb565_le);
