#include "PassportBridge.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static int s_battery = 73;
static int s_battery_reads;
static int s_ui_count;
static int s_render_count;
static uint8_t s_backlight;
static bool s_page_open;

int bsp_battery_soc(void) { ++s_battery_reads; return s_battery; }
void bsp_display_backlight(uint8_t percent) { s_backlight = percent; }
void passport_ui_create(int32_t battery) {
    assert(!s_page_open);
    assert(battery == s_battery);
    s_page_open = true;
}
void passport_ui_render(int32_t count, uint8_t backlight) {
    assert(s_page_open);
    assert(s_backlight == backlight);
    s_ui_count = count;
    ++s_render_count;
}
void passport_ui_destroy(void) {
    assert(s_page_open);
    assert(s_backlight == 100);
    s_page_open = false;
}

int main(void) {
    // Run the actual Embedded Swift adapter, with the two hardware APIs stubbed.
    // Repeat page lifecycles to exercise initialization and cleanup boundaries.
    for (int scenario = 0; scenario < 2; ++scenario) {
        s_battery = scenario == 0 ? 73 : -1;
        passport_swift_prepare();
        for (int i = 0; i < 100; ++i) {
            passport_swift_enter();
            assert(s_ui_count == 0 && s_backlight == 100);
            passport_swift_action(PASSPORT_ACTION_UP);
            passport_swift_action(PASSPORT_ACTION_UP);
            passport_swift_action(PASSPORT_ACTION_DOWN);
            assert(s_ui_count == 1);
            passport_swift_action(PASSPORT_ACTION_OK);
            assert(s_backlight == 25);
            int rendered = s_render_count;
            passport_swift_action(-100);
            assert(s_render_count == rendered);
            assert(s_battery_reads == scenario + 1);
            passport_swift_exit();
            assert(!s_page_open && s_backlight == 100);
        }
    }
    puts("Embedded Swift C interop: PASS (200 page lifecycles, optional battery)");
    return 0;
}
