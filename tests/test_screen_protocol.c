#include "screen_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    assert(screen_fap_request("FAP_SCREENSHOT_V1"));
    assert(!screen_fap_request(NULL));
    assert(!screen_fap_request("FAP_SCREENSHOT_V1 extra"));
    assert(!screen_fap_request("FAP_SCREENSHOT_V10"));
    char header[80];
    assert(screen_fap_header(header, sizeof(header), 240, 320) == strlen(header));
    assert(strcmp(header, "FAP_SCREENSHOT_V1 240 320 RGB565LE 153600\n") == 0);
    assert(!screen_fap_header(header, 8, 240, 320));
    assert(!screen_fap_header(header, sizeof(header), 0, 320));
    assert(!screen_fap_header(header, sizeof(header), 4096, 4096));
    uint16_t next_y = 0;
    assert(!screen_fap_advance(&next_y, 240, 320, 1, 0, 239));
    assert(!screen_fap_advance(&next_y, 240, 320, 0, 1, 240));
    for (uint16_t y = 0; y < 320; ++y) {
        assert(screen_fap_advance(&next_y, 240, 320, 0, y, 240));
        assert(!screen_fap_advance(&next_y, 240, 320, 0, y, 240));
    }
    assert(!screen_fap_advance(&next_y, 240, 320, 0, 320, 240));
    uint32_t id = 7;
    assert(screen_request_id("FPS1 CAPTURE 0123ABcd", &id) && id == 0x0123abcd);
    const char *invalid[] = { "", "FPS1 CAPTURE -1234567", "FPS1 CAPTURE 0123456",
                              "FPS1 CAPTURE 012345678", "FPS1 CAPTURE 0123456z", "FPS1 CAPTURE 01234567 extra" };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        assert(!screen_request_id(invalid[i], &id));
    }
    assert(screen_crc32((const uint8_t *)"123456789", 9) == 0xcbf43926u);
    const uint8_t colors[] = { 0x00, 0xf8, 0xe0, 0x07, 0x1f, 0x00 }; // RGB565 red/green/blue, LE
    char row[160];
    size_t length = screen_encode_row(row, sizeof(row), 0x12345678, 0, 0, 3, colors);
    assert(length > 0 && length == strlen(row));
    assert(strstr(row, " APjgBx8A\n") != NULL);
    assert(screen_encode_row(row, 8, 1, 0, 0, 3, colors) == 0);
    assert(screen_encode_row(row, sizeof(row), 1, 0, 0, 0, colors) == 0);
    // Emit actual firmware records; Python consumes these in the cross-language test.
    puts("FPS1 BEGIN 12345678 3 1 RGB565LE");
    length = screen_encode_row(row, sizeof(row), 0x12345678, 0, 0, 3, colors);
    fwrite(row, 1, length, stdout);
    puts("FPS1 END 12345678 3");
    return 0;
}
