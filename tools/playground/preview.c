#include "lvgl.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wasi/api.h>

static uint8_t draw_buffer[240 * 20 * 2];
static uint16_t framebuffer[240 * 320];
static uint32_t frame_revision;
static lv_display_t *display;
void swift_preview_init(void);
void swift_preview_button(int32_t button);
void swift_preview_tick(uint32_t milliseconds);

// Swift's bare-metal runtime requests entropy using this C ABI.
void arc4random_buf(void *buffer, size_t count) {
    if (__wasi_random_get(buffer, count)) abort();
}

static void flush(lv_display_t *d, const lv_area_t *area, uint8_t *pixels) {
    const lv_draw_buf_t *buffer = lv_display_get_buf_active(d);
    for (int y = area->y1; y <= area->y2; ++y) {
        memcpy(&framebuffer[y * 240 + area->x1],
               pixels + (y - area->y1) * buffer->header.stride,
               (area->x2 - area->x1 + 1) * 2);
    }
    ++frame_revision;
    lv_display_flush_ready(d);
}

void preview_init(void) {
    lv_init();
    display = lv_display_create(240, 320);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof(draw_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    swift_preview_init();
    lv_refr_now(display);
}
void preview_button(int32_t button) {
    swift_preview_button(button);
    lv_refr_now(display);
}
void preview_tick(uint32_t milliseconds) {
    lv_tick_inc(milliseconds);
    swift_preview_tick(milliseconds);
    lv_timer_handler();
}
uint16_t *preview_framebuffer(void) { return framebuffer; }
uint32_t preview_revision(void) { return frame_revision; }
