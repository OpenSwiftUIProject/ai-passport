#include "screen_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
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
