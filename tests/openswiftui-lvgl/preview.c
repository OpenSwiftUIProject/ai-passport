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
static unsigned s_flushed_pixels;
int bsp_battery_soc(void) { return 73; } // Explicit host fixture, not board data.
void bsp_display_backlight(uint8_t value) { (void)value; }
void passport_ui_create(int32_t value) { (void)value; }
void passport_ui_render(int32_t value, uint8_t light) { (void)value; (void)light; }
void passport_ui_destroy(void) {}
void passport_2048_fixture(int32_t scenario);
void passport_2048_animation_fixture(void);

static void advance(unsigned milliseconds)
{
    lv_tick_inc(milliseconds);
    lv_timer_handler();
}

static lv_obj_t *find_label(lv_obj_t *scene, const char *text, unsigned occurrence)
{
    for (unsigned i = 0; i < lv_obj_get_child_count(scene); ++i) {
        lv_obj_t *child = lv_obj_get_child(scene, i);
        if (lv_obj_check_type(child, &lv_label_class) && lv_obj_get_y(child) >= 22 && !strcmp(lv_label_get_text(child), text)) {
            if (occurrence-- == 0) return child;
        }
    }
    return NULL;
}

static bool has_label(lv_obj_t *scene, const char *text)
{
    for (unsigned i = 0; i < lv_obj_get_child_count(scene); ++i) {
        lv_obj_t *child = lv_obj_get_child(scene, i);
        if (lv_obj_check_type(child, &lv_label_class) && strcmp(lv_label_get_text(child), text) == 0) return true;
    }
    return false;
}

static lv_obj_t *checked_game_scene(void)
{
    lv_obj_t *scene = lv_obj_get_child(lv_screen_active(), -1);
    assert(scene && !has_label(scene, "View render failed"));
    assert(lv_obj_get_child_count(scene) >= 26 && lv_obj_get_child_count(scene) <= 58);
    lv_obj_update_layout(scene);
    lv_area_t bounds;
    lv_obj_get_coords(scene, &bounds);
    for (unsigned i = 0; i < lv_obj_get_child_count(scene); ++i) {
        lv_obj_t *child = lv_obj_get_child(scene, i);
        lv_area_t rect;
        lv_obj_get_coords(child, &rect);
        if (rect.y1 < bounds.y1 || rect.y2 > bounds.y2) {
            fprintf(stderr, "2048 child %u: (%ld,%ld)-(%ld,%ld), bounds (%ld,%ld)-(%ld,%ld)\n", i,
                    (long)rect.x1, (long)rect.y1, (long)rect.x2, (long)rect.y2,
                    (long)bounds.x1, (long)bounds.y1, (long)bounds.x2, (long)bounds.y2);
        }
        assert(rect.x1 >= bounds.x1 && rect.x2 <= bounds.x2);
        assert(rect.y1 >= bounds.y1 && rect.y2 <= bounds.y2);
        // Four-digit tile labels must remain a single measured line.
        if (lv_obj_check_type(child, &lv_label_class)) assert(lv_obj_get_height(child) == lv_font_montserrat_14.line_height);
    }
    return scene;
}

static void short_ok(void)
{
    demo_2048_key(BSP_BTN_OK, BSP_BTN_PRESS);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
}

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    s_flushed_pixels += (unsigned)lv_area_get_size(area);
    if (!s_wire) { lv_display_flush_ready(display); return; }
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

static void capture(lv_display_t *display, const char *path)
{
    s_wire = fopen(path, "wb");
    assert(s_wire);
    fputs("FPS1 BEGIN 12345678 240 320 RGB565LE\n", s_wire);
    s_pixels = 0;
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(display);
    assert(s_pixels == 76800);
    fprintf(s_wire, "FPS1 END 12345678 %u\n", s_pixels);
    assert(fclose(s_wire) == 0);
    s_wire = NULL;
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
    capture(display, argv[1]);
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
    // Drive the real page's physical-key adapter, verifying visual state and
    // ownership without needing ADC hardware. Only CLICK changes the scene.
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *scene = lv_obj_get_child(screen, -1);
    assert(lv_obj_get_child_count(scene) == 8);
    lv_obj_t *first = lv_obj_get_child(scene, 0);
    demo_openswiftui_key(BSP_BTN_DOWN, BSP_BTN_PRESS);
    demo_openswiftui_key(BSP_BTN_DOWN, BSP_BTN_DOUBLE);
    demo_openswiftui_key(BSP_BTN_OK, BSP_BTN_LONG);
    demo_openswiftui_key((bsp_btn_t)99, BSP_BTN_CLICK);
    assert(lv_obj_get_child(scene, 0) == first);
    demo_openswiftui_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
    assert(lv_screen_active() == screen && lv_obj_get_child(screen, -1) == scene);
    assert(lv_color_eq(lv_obj_get_style_bg_color(lv_obj_get_child(scene, 1), 0), lv_color_hex(0x244d38)));
    char action_path[1024];
    assert(snprintf(action_path, sizeof(action_path), "%s.down", argv[1]) < (int)sizeof(action_path));
    capture(display, action_path);
    demo_openswiftui_key(BSP_BTN_UP, BSP_BTN_CLICK);
    assert(lv_color_eq(lv_obj_get_style_bg_color(lv_obj_get_child(scene, 1), 0), lv_color_hex(0x243857)));
    demo_openswiftui_key(BSP_BTN_OK, BSP_BTN_CLICK);
    assert(lv_obj_get_child_count(scene) == 7);
    assert(snprintf(action_path, sizeof(action_path), "%s.hidden", argv[1]) < (int)sizeof(action_path));
    capture(display, action_path);
    demo_openswiftui_key(BSP_BTN_OK, BSP_BTN_CLICK);
    assert(lv_obj_get_child_count(scene) == 8);
    for (unsigned i = 0; i < 500; ++i) {
        demo_openswiftui_key(BSP_BTN_OK, BSP_BTN_CLICK);
        assert(lv_obj_get_child_count(scene) == (i % 2 ? 8 : 7));
    }
    // Exercise real LVGL sink rejection and lifecycle behavior after capture.
    assert(!passport_scene_begin(73));
    assert(!passport_scene_image(0, 0, 16, 16, (const uint8_t *)"wrong", 5));
    assert(!passport_scene_fill(0, 0, 0, 1, 0, 255));
    for (unsigned i = 8; i < 32; ++i) assert(passport_scene_fill(0, 0, 1, 1, 0, 255));
    assert(!passport_scene_fill(0, 0, 1, 1, 0, 255));
    demo_openswiftui_exit();
    for (unsigned i = 0; i < 20; ++i) {
        demo_openswiftui_enter();
        scene = lv_obj_get_child(lv_screen_active(), -1);
        assert(lv_obj_get_child_count(scene) == 8);
        assert(lv_color_eq(lv_obj_get_style_bg_color(lv_obj_get_child(scene, 1), 0), lv_color_hex(0x243857)));
        demo_openswiftui_key(BSP_BTN_OK, BSP_BTN_CLICK);
        demo_openswiftui_exit();
        demo_openswiftui_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
        assert(!passport_scene_reset());
    }
    demo_2048_enter();
    scene = checked_game_scene();
    assert(has_label(scene, "VERT  UP:^ DOWN:v"));
    assert(passport_scene_get_geometry(&geometry) && geometry.top == 46 && geometry.bottom == 34);
    assert(snprintf(action_path, sizeof(action_path), "%s.2048", argv[1]) < (int)sizeof(action_path));
    capture(display, action_path);
    // A stale release/click from entering the page cannot toggle its axis.
    demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_CLICK);
    assert(has_label(scene, "VERT  UP:^ DOWN:v"));
    short_ok();
    assert(has_label(checked_game_scene(), "HORIZ UP:< DOWN:>"));
    assert(snprintf(action_path, sizeof(action_path), "%s.2048-horizontal", argv[1]) < (int)sizeof(action_path));
    capture(display, action_path);
    short_ok(); // Two rapid taps remain two toggles, even with a following DOUBLE.
    demo_2048_key(BSP_BTN_OK, BSP_BTN_DOUBLE);
    assert(has_label(checked_game_scene(), "VERT  UP:^ DOWN:v"));
    demo_2048_key(BSP_BTN_OK, BSP_BTN_PRESS);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_LONG);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
    assert(has_label(checked_game_scene(), "VERT  UP:^ DOWN:v"));
    // Rapid directional PRESS notifications are accepted independently. Later
    // click/double/long/release notifications must not produce another render.
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_PRESS);
    scene = checked_game_scene();
    first = lv_obj_get_child(scene, 0);
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_RELEASE);
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_DOUBLE);
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_LONG);
    assert(lv_obj_get_child(scene, 0) == first);
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_PRESS);
    checked_game_scene();
    for (unsigned tick = 0; tick < 40; ++tick) advance(16); // Drain both queued moves.
    for (unsigned i = 0; i < 500; ++i) {
        short_ok();
        checked_game_scene();
    }
    demo_2048_exit();
    for (int32_t fixture = 0; fixture < 3; ++fixture) {
        passport_2048_fixture(fixture);
        scene = checked_game_scene();
        assert(lv_obj_get_child_count(scene) == 58);
        const char *suffix = fixture == 0 ? "tiles" : (fixture == 1 ? "won" : "lost");
        assert(has_label(scene, fixture == 0 ? "VERT  UP:^ DOWN:v" : (fixture == 1 ? "2048! YOU WIN" : "NO MOVES LEFT")));
        if (fixture == 1) assert(has_label(scene, "2048"));
        assert(snprintf(action_path, sizeof(action_path), "%s.2048-%s", argv[1], suffix) < (int)sizeof(action_path));
        capture(display, action_path);
        passport_scene_destroy();
    }
    for (unsigned i = 0; i < 20; ++i) {
        demo_2048_enter();
        assert(has_label(checked_game_scene(), "VERT  UP:^ DOWN:v"));
        short_ok();
        demo_2048_key(BSP_BTN_OK, BSP_BTN_PRESS);
        demo_2048_exit();
        demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
        assert(!passport_scene_reset());
    }
    // Capture exact animation phases using the actual page host, clock and sink.
    demo_2048_enter();
    passport_2048_animation_fixture();
    scene = checked_game_scene();
    lv_obj_t *moving_two = find_label(scene, "2", 1);
    assert(moving_two);
    int32_t source_y = lv_obj_get_y(moving_two);
    assert(snprintf(action_path, sizeof(action_path), "%s.2048-animation-0", argv[1]) < (int)sizeof(action_path));
    capture(display, action_path);
    // An input can arrive long after the previous clock callback. That idle
    // time must not skip the movement's first intermediate frame.
    lv_tick_inc(200);
    demo_2048_key(BSP_BTN_UP, BSP_BTN_PRESS);
    assert(lv_obj_get_y(moving_two) == source_y);
    for (unsigned frame = 1; frame <= 3; ++frame) {
        advance(80);
        checked_game_scene();
        assert(snprintf(action_path, sizeof(action_path), "%s.2048-animation-%u", argv[1], frame) < (int)sizeof(action_path));
        capture(display, action_path);
        if (frame == 1) {
            assert(lv_obj_get_y(moving_two) < source_y && lv_obj_get_y(moving_two) > source_y - 92);
            assert(find_label(scene, "2", 1) == moving_two); // No delete/recreate during movement.
        }
    }
    advance(80);
    checked_game_scene();
    assert(snprintf(action_path, sizeof(action_path), "%s.2048-animation-4", argv[1]) < (int)sizeof(action_path));
    capture(display, action_path);
    advance(80);
    checked_game_scene();
    assert(has_label(scene, "8"));
    assert(snprintf(action_path, sizeof(action_path), "%s.2048-animation-5", argv[1]) < (int)sizeof(action_path));
    capture(display, action_path);

    // Additional 16 ms snapshots are an actual host-rendered animation sequence.
    passport_2048_animation_fixture();
    demo_2048_key(BSP_BTN_UP, BSP_BTN_PRESS);
    for (unsigned frame = 0; frame < 27; ++frame) {
        if (frame) advance(16);
        assert(snprintf(action_path, sizeof(action_path), "%s.2048-motion-%02u", argv[1], frame) < (int)sizeof(action_path));
        capture(display, action_path);
    }
    scene = checked_game_scene();
    // Queue a direction followed by an axis change; both eventually execute.
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_PRESS);
    short_ok();
    assert(has_label(scene, "VERT  UP:^ DOWN:v"));
    for (unsigned tick = 0; tick < 40; ++tick) advance(16);
    assert(has_label(scene, "HORIZ UP:< DOWN:>"));
    for (unsigned round = 0; round < 200; ++round) {
        demo_2048_key(round % 2 ? BSP_BTN_UP : BSP_BTN_DOWN, BSP_BTN_PRESS);
        if (round % 3 == 0) short_ok();
        for (unsigned tick = 0; tick < 40; ++tick) advance(16);
        checked_game_scene();
    }
    // Saturate the bounded queue, then exit while the first move is animating.
    passport_2048_animation_fixture();
    demo_2048_key(BSP_BTN_UP, BSP_BTN_PRESS);
    for (unsigned i = 0; i < 12; ++i) demo_2048_key(BSP_BTN_DOWN, BSP_BTN_PRESS);
    demo_2048_exit();
    // The app immediately loads its menu after page exit. Supply that live
    // destination screen before allowing LVGL's display refresh timer to run.
    lv_obj_t *menu = lv_obj_create(NULL);
    lv_screen_load(menu);
    for (unsigned tick = 0; tick < 40; ++tick) advance(16);
    assert(!passport_scene_reset());
    demo_2048_enter();
    lv_obj_delete(menu);
    for (unsigned tick = 0; tick < 40; ++tick) advance(16);
    assert(has_label(checked_game_scene(), "VERT  UP:^ DOWN:v"));
    demo_2048_exit();
    puts("2048 animation: PASS (intermediate pixels, merge/reveal phases, object reuse, FIFO, saturation, 200 moves, exit during animation)");
    assert(passport_scene_begin(73));
    for (unsigned pass = 0; pass < 2; ++pass) {
        assert(passport_scene_reset());
        assert(passport_scene_fill(0, 0, 80, 40, 0x123456, 255));
        assert(passport_scene_text(0, 4, 80, 16, (const uint8_t *)"2048", 4, 0xffffff, 255));
        passport_scene_end(true);
        s_flushed_pixels = 0;
        lv_refr_now(display);
        if (pass == 0) assert(s_flushed_pixels > 0);
        else assert(s_flushed_pixels == 0); // Repeated values must not invalidate the display.
    }
    passport_scene_destroy();
    lv_mem_monitor_t memory;
    lv_mem_monitor(&memory);
    assert(lv_mem_test() == LV_RESULT_OK);
    printf("2048 LVGL: PASS (axis/input/lifecycle, full board/end states, 500 redraws, pool peak=%zu/%zu bytes)\n",
           memory.max_used, memory.total_size);
    // Root configuration follows the real LVGL display, not duplicated Swift constants.
    lv_display_set_resolution(display, 320, 360);
    demo_openswiftui_enter();
    assert(passport_scene_get_geometry(&geometry));
    assert(geometry.screen_width == 320 && geometry.screen_height == 360);
    demo_openswiftui_exit();
    lv_display_delete(display);
    lv_deinit();
    puts("Headless LVGL: PASS (76800 pixels, real C sink, real font wrapping, display resize, three button closures, 500 redraws, asset rejection, node limit, 20 state resets)");
    return 0;
}
