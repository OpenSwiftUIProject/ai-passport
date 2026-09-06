#include "demo.h"
#include "swift/PassportBridge.h"
#include "ui_pixel.h"
#include "esp_log.h"
#include "lvgl.h"

// All handles are page-owned and only touched while the menu holds the LVGL
// lock. There are no page workers/timers or deferred references to tear down.
static lv_obj_t *s_screen;
static lv_obj_t *s_count;
static lv_obj_t *s_backlight;

void passport_ui_create(int32_t boot_battery_percent)
{
    s_screen = ui_pixel_screen_create("SWIFT");

    // Keep the battery snapshot below the cloud and above the content panel.
    lv_obj_t *battery = ui_pixel_label(s_screen, "", &lv_font_montserrat_14, UI_PAPER);
    lv_obj_align(battery, LV_ALIGN_TOP_RIGHT, -12, 46);
    if (boot_battery_percent >= 0 && boot_battery_percent <= 100) {
        lv_label_set_text_fmt(battery, "Boot: %ld%%", (long)boot_battery_percent);
    } else {
        lv_label_set_text(battery, "Boot: --%");
    }

    lv_obj_t *panel = ui_pixel_panel_create(s_screen, 18, 65, 204, 164, UI_PAPER);
    lv_obj_t *title = ui_pixel_label(panel, "Hello from Swift", &lv_font_montserrat_14, UI_INK);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    s_count = ui_pixel_label(panel, "0", &lv_font_montserrat_20, UI_SKY_DARK);
    lv_obj_align(s_count, LV_ALIGN_TOP_MID, 0, 38);
    s_backlight = ui_pixel_label(panel, "", &lv_font_montserrat_14, UI_INK);
    lv_obj_align(s_backlight, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_t *hint = ui_pixel_label(panel, "UP / DOWN: count\nOK: dim / brighten", &lv_font_montserrat_14, UI_INK);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 104);
    ui_pixel_mascot_create(s_screen, 101, 238);
    lv_screen_load(s_screen);
}

void passport_ui_render(int32_t count, uint8_t backlight)
{
    if (!s_screen) return;
    lv_label_set_text_fmt(s_count, "%ld", (long)count);
    lv_label_set_text_fmt(s_backlight, "Backlight: %u%%", (unsigned)backlight);
    ESP_LOGI("swift_demo", "count=%ld backlight=%u", (long)count, (unsigned)backlight);
}

void passport_ui_destroy(void)
{
    if (s_screen) lv_obj_delete(s_screen);
    s_screen = s_count = s_backlight = NULL;
}

void demo_swift_enter(void) { passport_swift_enter(); }
void demo_swift_exit(void) { passport_swift_exit(); }

void demo_swift_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    // A click is counted once; PRESS/DOUBLE/LONG are deliberately ignored.
    // OK-long is intercepted by main.c and exits back to the menu.
    if (ev != BSP_BTN_CLICK) return;
    switch (btn) {
    case BSP_BTN_UP: passport_swift_action(PASSPORT_ACTION_UP); break;
    case BSP_BTN_DOWN: passport_swift_action(PASSPORT_ACTION_DOWN); break;
    case BSP_BTN_OK: passport_swift_action(PASSPORT_ACTION_OK); break;
    default: break;
    }
}
