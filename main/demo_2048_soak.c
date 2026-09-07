// Optional, explicitly selected diagnostic; excluded from production behavior.
#ifdef PASSPORT_2048_SOAK
#include "demo.h"
#include "physical_input.h"
#include "swift/PassportBridge.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void passport_2048_soak_setup(uint32_t scenario);
static unsigned s_step;
static void soak_tick(lv_timer_t *timer)
{
    if (s_step >= 1200) {
        lv_timer_delete(timer);
        ESP_LOGI("2048-soak", "PASS steps=%u free_heap=%lu stack_free=%lu",
                 s_step, (unsigned long)esp_get_free_heap_size(),
                 (unsigned long)uxTaskGetStackHighWaterMark(NULL));
        return;
    }
    if (s_step % 80 == 0) {
        passport_2048_soak_setup(s_step / 80);
    } else if (s_step % 7 == 0) {
        physical_input_enqueue(BSP_BTN_OK, BSP_BTN_PRESS, NULL);
        physical_input_enqueue(BSP_BTN_OK, BSP_BTN_RELEASE, NULL);
    } else {
        physical_input_enqueue(s_step % 3 ? BSP_BTN_DOWN : BSP_BTN_UP, BSP_BTN_PRESS, NULL);
    }
    if (s_step % 40 == 0) {
        lv_mem_monitor_t pool;
        lv_mem_monitor(&pool);
        ESP_LOGI("2048-soak", "step=%u heap=%lu stack_free=%lu pool_used=%lu pool_largest=%lu integrity=%s",
                 s_step, (unsigned long)esp_get_free_heap_size(),
                 (unsigned long)uxTaskGetStackHighWaterMark(NULL),
                 (unsigned long)(pool.total_size - pool.free_size), (unsigned long)pool.free_biggest_size,
                 lv_mem_test() == LV_RESULT_OK ? "OK" : "FAIL");
    }
    ++s_step;
}
void demo_2048_soak_start(void)
{
    s_step = 0;
    lv_timer_create(soak_tick, 90, NULL);
}
#endif
