#include "PassportBridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned s_calls, s_fail_at, s_renders;
static unsigned s_expected_palette;
static bool s_expected_image = true, s_reset_ok = true;
static const uint32_t PALETTES[] = {0x243857, 0x244d38, 0x4d2957};
static bool s_begin = true, s_ended, s_success;
static bool s_geometry_ok = true, s_measure_ok = true;
bool passport_scene_get_geometry(passport_scene_geometry_t *out) {
    *out = (passport_scene_geometry_t){240, 320, 66, 12, 30, 12};
    return s_geometry_ok;
}
bool passport_scene_measure_image(const uint8_t *name, uint32_t n, int32_t *w, int32_t *h) {
    assert(n == 5 && memcmp(name, "spark", 5) == 0);
    *w = *h = 16; return s_measure_ok;
}
bool passport_scene_measure_text(const uint8_t *text, uint32_t n, int32_t proposed, int32_t *w, int32_t *h) {
    (void)text; (void)proposed;
    *w = n * 7; *h = 14; return s_measure_ok; // Explicit host font fixture.
}
int bsp_battery_soc(void) { return 73; }
void bsp_display_backlight(uint8_t value) { (void)value; }
void passport_ui_create(int32_t value) { (void)value; }
void passport_ui_render(int32_t value, uint8_t light) { (void)value; (void)light; }
void passport_ui_destroy(void) {}

bool passport_scene_begin(int32_t battery) { assert(battery == 73); return s_begin; }
bool passport_scene_reset(void) {
    s_calls = 0; s_ended = false; s_success = false;
    return s_reset_ok;
}
bool passport_scene_fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t rgb, uint8_t alpha)
{
    ++s_calls;
    assert(alpha == 255);
    unsigned position = s_calls + (!s_expected_image && s_calls >= 3);
    if (position == 1) assert(x == 4 && y == 11 && w == 208 && h == 202 && rgb == 0x0e1729);
    else if (position == 2) assert(x == 16 && y == 23 && w == 184 && h == 112 && rgb == PALETTES[s_expected_palette]);
    else if (position == 6) assert(x == 44 && y == 193 && w == 32 && h == 8 && rgb == 0xff0000);
    else if (position == 7) assert(x == 92 && y == 193 && w == 32 && h == 8 && rgb == 0x00ff00);
    else if (position == 8) assert(x == 140 && y == 193 && w == 32 && h == 8 && rgb == 0x0000ff);
    else assert(false);
    return s_calls != s_fail_at;
}
bool passport_scene_image(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *name, uint32_t n)
{
    ++s_calls;
    assert(s_expected_image && s_calls == 3 && x == 68 && y == 39 && w == 80 && h == 80);
    assert(n == 5 && memcmp(name, "spark", 5) == 0);
    return s_calls != s_fail_at;
}
bool passport_scene_text(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *text, uint32_t n, uint32_t rgb, uint8_t alpha)
{
    ++s_calls;
    assert(alpha == 255);
    unsigned position = s_calls + !s_expected_image;
    if (position == 4) {
        assert(x == 41 && y == 147 && w == 133 && h == 14 && rgb == 0xffffff);
        assert(n == 19 && memcmp(text, "Hello, OpenSwiftUI!", 19) == 0);
    } else {
        assert(position == 5 && x == 48 && y == 167 && w == 119 && h == 14 && rgb == 0xffff00);
        assert(n == 17 && memcmp(text, "Swift on ESP32-C3", 17) == 0);
    }
    return s_calls != s_fail_at;
}
void passport_scene_end(bool succeeded) { s_ended = true; s_success = succeeded; ++s_renders; }

int main(void)
{
    assert(openswiftui_embedded_version() == 4);
    passport_swift_prepare();
    for (unsigned failure = 0; failure <= 8; ++failure) {
        s_calls = 0; s_ended = false; s_success = false; s_fail_at = failure;
        passport_content_enter();
        assert(s_calls == (failure ? failure : 8));
        assert(s_ended && s_success == (failure == 0));
        passport_content_exit();
    }
    s_begin = false; s_calls = 0; s_ended = false;
    passport_content_enter();
    assert(s_calls == 0 && !s_ended);
    passport_content_exit();
    s_begin = true; s_geometry_ok = false; s_calls = 0; s_ended = false;
    passport_content_enter();
    assert(s_calls == 0 && s_ended && !s_success);
    passport_content_exit();
    s_geometry_ok = true; s_measure_ok = false; s_calls = 0; s_ended = false;
    passport_content_enter();
    assert(s_calls == 0 && s_ended && !s_success);
    passport_content_exit();
    s_measure_ok = true; s_reset_ok = false; s_calls = 0;
    passport_content_enter();
    assert(s_calls == 0 && !s_success);
    passport_content_exit();
    s_reset_ok = true; s_fail_at = 0;
    passport_content_enter();
    assert(s_success && s_calls == 8);
    unsigned previous = s_renders;
    passport_content_enter(); // Duplicate entry preserves host/state and screen.
    passport_content_action(-1);
    assert(s_renders == previous);
    s_expected_palette = 1;
    passport_content_action(PASSPORT_ACTION_DOWN);
    assert(s_success && s_renders == previous + 1);
    s_expected_palette = 0;
    passport_content_action(PASSPORT_ACTION_UP);
    assert(s_success);
    s_expected_palette = 2;
    passport_content_action(PASSPORT_ACTION_UP);
    assert(s_success);
    s_expected_image = false;
    passport_content_action(PASSPORT_ACTION_OK);
    assert(s_success && s_calls == 7);
    s_expected_image = true;
    passport_content_action(PASSPORT_ACTION_OK);
    assert(s_success && s_calls == 8);
    // Recover from a failed redraw without recreating the page or resetting state.
    s_fail_at = 2; s_expected_palette = 0;
    passport_content_action(PASSPORT_ACTION_DOWN);
    assert(!s_success && s_calls == 2);
    s_fail_at = 0; s_expected_palette = 1;
    passport_content_action(PASSPORT_ACTION_DOWN);
    assert(s_success && s_calls == 8);
    for (unsigned i = 0; i < 500; ++i) {
        s_expected_palette = (s_expected_palette + 1) % 3;
        passport_content_action(PASSPORT_ACTION_DOWN);
        assert(s_success && s_calls == 8);
    }
    passport_content_exit(); previous = s_renders;
    passport_content_action(PASSPORT_ACTION_OK);
    assert(s_renders == previous);
    s_expected_palette = 0; s_expected_image = true;
    passport_content_enter();
    assert(s_success && s_calls == 8);
    passport_content_exit();
    puts("Passport ContentView interop: PASS (real State closures, three actions, layout, 500 redraws, failures, exit/reentry)");
    return 0;
}
