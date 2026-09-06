#include "lvgl.h"
#include "demo.h"
#include "screen_protocol.h"
#include "swift/PassportBridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t s_buffer[240 * 20 * 2];
static FILE *s_wire;
static unsigned s_pixels;
int bsp_battery_soc(void) { return 73; } // Explicit host fixture, not board data.
void bsp_display_backlight(uint8_t value) { (void)value; }
void passport_ui_create(int32_t value) { (void)value; }
void passport_ui_render(int32_t value, uint8_t light) { (void)value; (void)light; }
void passport_ui_destroy(void) {}

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    assert(area->x1 >= 0 && area->x2 < 240 && area->y1 >= 0 && area->y2 < 320);
    const lv_draw_buf_t *buffer = lv_display_get_buf_active(display);
    unsigned width = lv_area_get_width(area), height = lv_area_get_height(area);
    char packet[800];
    for (unsigned row = 0; row < height; ++row) {
        size_t size = screen_encode_row(packet, sizeof(packet), 0x12345678,
                                        area->x1, area->y1 + row, width,
                                        pixels + row * buffer->header.stride);
        assert(size > 0 && fwrite(packet, 1, size, s_wire) == size);
        s_pixels += width;
    }
    lv_display_flush_ready(display);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    lv_init();
    lv_display_t *display = lv_display_create(240, 320);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, s_buffer, NULL, sizeof(s_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    passport_swift_prepare();
    demo_openswiftui_enter();
    s_wire = fopen(argv[1], "wb");
    assert(s_wire);
    fputs("FPS1 BEGIN 12345678 240 320 RGB565LE\n", s_wire);
    lv_refr_now(display);
    assert(s_pixels == 76800);
    fprintf(s_wire, "FPS1 END 12345678 %u\n", s_pixels);
    assert(fclose(s_wire) == 0);
    passport_scene_geometry_t geometry;
    assert(passport_scene_get_geometry(&geometry));
    assert(geometry.screen_width == 240 && geometry.screen_height == 320);
    assert(geometry.leading == 12 && geometry.top == 66 && geometry.bottom == 30);
    int32_t natural_w, natural_h, wrapped_w, wrapped_h;
    const uint8_t *caption = (const uint8_t *)"Hello, OpenSwiftUI!";
    assert(passport_scene_measure_text(caption, 19, -1, &natural_w, &natural_h));
    assert(passport_scene_measure_text(caption, 19, 60, &wrapped_w, &wrapped_h));
    assert(natural_w > 60 && wrapped_w <= 60 && wrapped_h > natural_h);
    assert(!passport_scene_measure_text(caption, 96, 60, &wrapped_w, &wrapped_h));
    assert(!passport_scene_measure_text(caption, 19, -2, &wrapped_w, &wrapped_h));
    assert(!passport_scene_measure_image((const uint8_t *)"wrong", 5, &natural_w, &natural_h));
    // Compare the measurement against an actual wrapped LVGL label.
    lv_obj_t *probe = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(probe, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(probe, 0, 0);
    lv_obj_set_style_text_line_space(probe, 0, 0);
    lv_obj_set_width(probe, 60);
    lv_label_set_long_mode(probe, LV_LABEL_LONG_WRAP);
    lv_label_set_text(probe, (const char *)caption);
    lv_obj_update_layout(probe);
    assert(lv_obj_get_height(probe) == wrapped_h);
    lv_obj_delete(probe);
    // Exercise real LVGL sink rejection and lifecycle behavior after capture.
    assert(!passport_scene_begin(73));
    assert(!passport_scene_image(0, 0, 16, 16, (const uint8_t *)"wrong", 5));
    assert(!passport_scene_fill(0, 0, 0, 1, 0, 255));
    for (unsigned i = 8; i < 32; ++i) assert(passport_scene_fill(0, 0, 1, 1, 0, 255));
    assert(!passport_scene_fill(0, 0, 1, 1, 0, 255));
    demo_openswiftui_exit();
    for (unsigned i = 0; i < 20; ++i) {
        demo_openswiftui_enter();
        demo_openswiftui_exit();
    }
    // Root configuration follows the real LVGL display, not duplicated Swift constants.
    lv_display_set_resolution(display, 320, 360);
    demo_openswiftui_enter();
    assert(passport_scene_get_geometry(&geometry));
    assert(geometry.screen_width == 320 && geometry.screen_height == 360);
    demo_openswiftui_exit();
    lv_display_delete(display);
    lv_deinit();
    puts("Headless LVGL: PASS (76800 pixels, real C sink, real font wrapping, display resize, asset rejection, node limit, 20 lifecycles)");
    return 0;
}
