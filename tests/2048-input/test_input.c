#include "demo.h"
#include "swift/PassportBridge.h"
#include <assert.h>
#include <stdio.h>

static unsigned s_entries, s_exits, s_destroyed, s_count;
static int32_t s_actions[16];
void passport_2048_enter(uint32_t seed) { assert(seed == 0x2048); ++s_entries; }
void passport_2048_exit(void) { ++s_exits; }
void passport_scene_destroy(void) { ++s_destroyed; }
void passport_2048_action(int32_t action) { assert(s_count < 16); s_actions[s_count++] = action; }

int main(void)
{
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_PRESS);
    assert(s_count == 0);
    demo_2048_enter();
    demo_2048_enter();
    assert(s_entries == 1);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_CLICK);
    assert(s_count == 0); // Menu-entry tail.
    for (unsigned i = 0; i < 2; ++i) {
        demo_2048_key(BSP_BTN_DOWN, BSP_BTN_PRESS);
        demo_2048_key(BSP_BTN_DOWN, BSP_BTN_RELEASE);
    }
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_DOUBLE);
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_CLICK);
    demo_2048_key(BSP_BTN_DOWN, BSP_BTN_LONG);
    assert(s_count == 2 && s_actions[0] == PASSPORT_ACTION_DOWN && s_actions[1] == PASSPORT_ACTION_DOWN);
    demo_2048_key(BSP_BTN_UP, BSP_BTN_PRESS);
    assert(s_count == 3 && s_actions[2] == PASSPORT_ACTION_UP);
    for (unsigned i = 0; i < 2; ++i) {
        demo_2048_key(BSP_BTN_OK, BSP_BTN_PRESS);
        demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
    }
    demo_2048_key(BSP_BTN_OK, BSP_BTN_DOUBLE);
    assert(s_count == 5 && s_actions[3] == PASSPORT_ACTION_OK && s_actions[4] == PASSPORT_ACTION_OK);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_PRESS);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_LONG);
    demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
    demo_2048_key((bsp_btn_t)99, BSP_BTN_PRESS);
    assert(s_count == 5);
    // The actual app intercepts LONG by exiting before any later RELEASE.
    demo_2048_key(BSP_BTN_OK, BSP_BTN_PRESS);
    demo_2048_exit();
    demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
    demo_2048_key(BSP_BTN_UP, BSP_BTN_PRESS);
    assert(s_count == 5 && s_exits == 1 && s_destroyed == 1);
    demo_2048_enter();
    demo_2048_key(BSP_BTN_OK, BSP_BTN_RELEASE);
    assert(s_count == 5 && s_entries == 2);
    demo_2048_exit();
    puts("2048 input: PASS (rapid directions/OK, no repeat, long-OK exclusion, menu tails, lifecycle)");
}
