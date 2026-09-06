#include "physical_input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include "esp_log.h"
#include <stdint.h>

typedef struct {
    bsp_btn_t button;
    bsp_btn_ev_t event;
    uint32_t generation;
} input_event_t;
#define INPUT_QUEUE_CAPACITY 16
static QueueHandle_t s_queue;
static physical_input_handler_t s_handler;
// Compiler atomics avoid the C++ stdatomic.h injected into the Swift/C target.
// Both the ESP GCC toolchain and the host Clang implement these builtins.
static uint32_t s_generation;
static unsigned s_dropped;

void physical_input_enqueue(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    uint32_t generation = __atomic_load_n(&s_generation, __ATOMIC_ACQUIRE);
    if (!generation) return;
    input_event_t input = {button, event, generation};
    if (xQueueSend(s_queue, &input, 0) != pdTRUE) {
        __atomic_fetch_add(&s_dropped, 1, __ATOMIC_RELAXED);
    }
}

// LVGL invokes this under its own lock; work per tick is bounded. Advancing
// pages from the handler invalidates remaining events captured on the old page.
static void drain_input(lv_timer_t *timer)
{
    (void)timer;
    if (!__atomic_load_n(&s_generation, __ATOMIC_ACQUIRE)) return;
    input_event_t input;
    for (unsigned i = 0; i < INPUT_QUEUE_CAPACITY; ++i) {
        if (xQueueReceive(s_queue, &input, 0) != pdTRUE) break;
        if (input.generation == __atomic_load_n(&s_generation, __ATOMIC_RELAXED)) {
            s_handler(input.button, input.event);
        }
    }
    unsigned dropped = __atomic_exchange_n(&s_dropped, 0, __ATOMIC_RELAXED);
    if (dropped) ESP_LOGW("input", "Queue full: dropped %u events", dropped);
}

bool physical_input_init(physical_input_handler_t handler)
{
    if (s_queue || !handler) return false;
    s_queue = xQueueCreate(INPUT_QUEUE_CAPACITY, sizeof(input_event_t));
    if (!s_queue) return false;
    s_handler = handler;
    if (!lv_timer_create(drain_input, 16, NULL)) {
        vQueueDelete(s_queue);
        s_queue = NULL;
        s_handler = NULL;
        return false;
    }
    return true;
}

void physical_input_start(void)
{
    uint32_t initial = 0;
    if (s_queue) __atomic_compare_exchange_n(&s_generation, &initial, 1, false,
                                                        __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

void physical_input_advance_page(void)
{
    if (!__atomic_load_n(&s_generation, __ATOMIC_RELAXED)) return;
    if (__atomic_fetch_add(&s_generation, 1, __ATOMIC_ACQ_REL) == UINT32_MAX) {
        __atomic_store_n(&s_generation, 1, __ATOMIC_RELEASE);
    }
}
