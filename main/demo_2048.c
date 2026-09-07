#include "demo.h"
#include "swift/PassportBridge.h"
#include "esp_random.h"

static bool s_active, s_ok_pressed;

void demo_2048_enter(void)
{
    if (s_active) return;
    s_active = true;
    s_ok_pressed = false;
    passport_2048_enter(esp_random());
}

void demo_2048_exit(void)
{
    s_active = s_ok_pressed = false;
    passport_2048_exit();
    passport_scene_destroy();
}

void demo_2048_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (!s_active) return;
    if (button == BSP_BTN_OK) {
        // Arm only on this page. The release used to enter from the menu must
        // not toggle the axis; long OK remains owned by the app's navigation.
        if (event == BSP_BTN_PRESS) s_ok_pressed = true;
        else if (event == BSP_BTN_LONG) s_ok_pressed = false;
        else if (event == BSP_BTN_RELEASE && s_ok_pressed) {
            s_ok_pressed = false;
            passport_2048_action(PASSPORT_ACTION_OK);
        }
    } else if (event == BSP_BTN_PRESS) {
        // One move per physical press, including rapid repeated presses.
        // Ignore click/double/long notifications for the same gesture.
        if (button == BSP_BTN_UP) passport_2048_action(PASSPORT_ACTION_UP);
        else if (button == BSP_BTN_DOWN) passport_2048_action(PASSPORT_ACTION_DOWN);
    }
}
