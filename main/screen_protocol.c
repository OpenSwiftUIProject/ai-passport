#include "screen_protocol.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

bool screen_fap_request(const char *line)
{
    return line && strcmp(line, "FAP_SCREENSHOT_V1") == 0;
}

size_t screen_fap_header(char *output, size_t capacity, uint16_t width, uint16_t height)
{
    if (!output || !capacity || !width || !height || width > 4096 || height > 4096 ||
        (uint32_t)width * height * 2 > 10 * 1024 * 1024) return 0;
    int length = snprintf(output, capacity, "FAP_SCREENSHOT_V1 %u %u RGB565LE %" PRIu32 "\n",
                          width, height, (uint32_t)width * height * 2);
    return length > 0 && (size_t)length < capacity ? (size_t)length : 0;
}

bool screen_fap_advance(uint16_t *next_y, uint16_t frame_width, uint16_t frame_height,
                        uint16_t x, uint16_t y, uint16_t width)
{
    if (!next_y || !frame_width || x || width != frame_width ||
        y != *next_y || y >= frame_height) return false;
    ++*next_y;
    return true;
}

bool screen_request_id(const char *line, uint32_t *id)
{
    const char prefix[] = "FPS1 CAPTURE ";
    if (!line || !id || strlen(line) != sizeof(prefix) - 1 + 8 ||
        strncmp(line, prefix, sizeof(prefix) - 1) != 0) return false;
    uint32_t value = 0;
    for (size_t i = sizeof(prefix) - 1; line[i]; ++i) {
        char c = line[i];
        uint32_t digit;
        if (c >= '0' && c <= '9') digit = (uint32_t)(c - '0');
        else if (c >= 'a' && c <= 'f') digit = (uint32_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') digit = (uint32_t)(c - 'A' + 10);
        else return false;
        value = (value << 4) | digit;
    }
    *id = value;
    return true;
}

uint32_t screen_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

size_t screen_encode_row(char *output, size_t capacity, uint32_t id,
                         uint16_t x, uint16_t y, uint16_t width,
                         const uint8_t *rgb565_le)
{
    if (!output || !rgb565_le || !width || !capacity) return 0;
    size_t length = (size_t)width * 2;
    int header = snprintf(output, capacity, "FPS1 ROW %08" PRIx32 " %u %u %u %08" PRIx32 " ",
                          id, (unsigned)x, (unsigned)y, (unsigned)width,
                          screen_crc32(rgb565_le, length));
    size_t encoded_size = 4 * ((length + 2) / 3);
    if (header < 0 || (size_t)header >= capacity ||
        encoded_size + 2 > capacity - (size_t)header) return 0;
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t cursor = (size_t)header;
    for (size_t i = 0; i < length; i += 3) {
        uint32_t word = (uint32_t)rgb565_le[i] << 16;
        if (i + 1 < length) word |= (uint32_t)rgb565_le[i + 1] << 8;
        if (i + 2 < length) word |= rgb565_le[i + 2];
        output[cursor++] = alphabet[(word >> 18) & 63];
        output[cursor++] = alphabet[(word >> 12) & 63];
        output[cursor++] = i + 1 < length ? alphabet[(word >> 6) & 63] : '=';
        output[cursor++] = i + 2 < length ? alphabet[word & 63] : '=';
    }
    output[cursor++] = '\n';
    output[cursor] = '\0';
    return cursor;
}
