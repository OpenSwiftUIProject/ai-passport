#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "bsp_battery.h"
#include "bsp_display.h"

// Application actions, intentionally independent of ADC/button encodings.
typedef enum {
    PASSPORT_ACTION_UP = 1,
    PASSPORT_ACTION_DOWN,
    PASSPORT_ACTION_OK,
} passport_action_t;

// C -> Swift. prepare runs once after BSP initialization, outside the LVGL lock.
// The other entry points run serialized under the existing menu's LVGL lock.
void passport_swift_prepare(void);
void passport_swift_enter(void);
void passport_swift_exit(void);
void passport_swift_action(int32_t action);

// Swift -> C. UI objects are owned by demo_swift.c; these require the LVGL lock.
void passport_ui_create(int32_t boot_battery_percent);
void passport_ui_render(int32_t count, uint8_t backlight);
void passport_ui_destroy(void);

// OpenSwiftUI display profile. All scene calls require the LVGL lock.
// UTF-8 inputs are length-delimited; C copies labels and resolves static assets.
uint32_t openswiftui_embedded_version(void);
void passport_content_enter(void);
void passport_content_exit(void);
void passport_content_action(int32_t action);
void passport_2048_enter(uint32_t seed);
void passport_2048_exit(void);
void passport_2048_action(int32_t action);
void passport_2048_tick(uint32_t elapsed_ms);
// Page-owned LVGL clock; stopped before releasing Swift state or scene objects.
bool passport_scene_start_clock(void (*callback)(uint32_t));
void passport_scene_stop_clock(void);
void passport_scene_clock_begin(void);
void passport_scene_clock_phase(void);
void passport_scene_clock_report(void);
void passport_scene_input_overflow(void);
typedef struct {
    int32_t screen_width, screen_height;
    int32_t top, leading, bottom, trailing;
} passport_scene_geometry_t;
bool passport_scene_begin(int32_t boot_battery_percent);
bool passport_scene_begin_2048(int32_t boot_battery_percent);
void passport_scene_destroy(void);
// Begins a frame, reusing compatible drawing objects in command order.
bool passport_scene_reset(void);
bool passport_scene_get_geometry(passport_scene_geometry_t *geometry);
bool passport_scene_measure_image(const uint8_t *name, uint32_t length, int32_t *width, int32_t *height);
bool passport_scene_measure_text(const uint8_t *text, uint32_t length, int32_t proposed_width, int32_t *width, int32_t *height);
bool passport_scene_fill(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb, uint8_t alpha);
bool passport_scene_image(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t *name, uint32_t length);
bool passport_scene_text(int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t *text, uint32_t length, uint32_t rgb, uint8_t alpha);
void passport_scene_end(bool succeeded);
