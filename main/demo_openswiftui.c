#include "demo.h"
#include "swift/PassportBridge.h"

void demo_openswiftui_enter(void) { passport_content_enter(); }
void demo_openswiftui_exit(void)
{
    passport_content_exit();
    passport_scene_destroy();
}
void demo_openswiftui_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    // A click executes exactly one closure. Long OK is owned by the app menu.
    if (ev != BSP_BTN_CLICK) return;
    switch (btn) {
    case BSP_BTN_UP: passport_content_action(PASSPORT_ACTION_UP); break;
    case BSP_BTN_DOWN: passport_content_action(PASSPORT_ACTION_DOWN); break;
    case BSP_BTN_OK: passport_content_action(PASSPORT_ACTION_OK); break;
    default: break;
    }
}
