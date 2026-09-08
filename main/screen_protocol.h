#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Hardware-independent, bounded wire format shared with host golden tests.
bool screen_request_id(const char *line, uint32_t *id);
bool screen_fap_request(const char *line);
size_t screen_fap_header(char *output, size_t capacity, uint16_t width, uint16_t height);
// A full invalidation must produce consecutive, full-width rows. Reject partial,
// reordered or duplicate rows instead of silently corrupting a binary capture.
bool screen_fap_advance(uint16_t *next_y, uint16_t frame_width, uint16_t frame_height,
                        uint16_t x, uint16_t y, uint16_t width);
uint32_t screen_crc32(const uint8_t *data, size_t length);
size_t screen_encode_row(char *output, size_t capacity, uint32_t id,
                         uint16_t x, uint16_t y, uint16_t width,
                         const uint8_t *rgb565_le);
