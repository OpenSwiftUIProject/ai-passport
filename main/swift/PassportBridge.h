#pragma once

#include <stdint.h>
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
