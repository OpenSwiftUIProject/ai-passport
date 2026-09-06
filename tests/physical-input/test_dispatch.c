#include "physical_input.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned char s_items[16][32];
static unsigned s_capacity, s_item_size, s_count, s_sent, s_deleted, s_warnings;
static unsigned s_delivered;
static bsp_btn_t s_buttons[32];
static bsp_btn_ev_t s_events[32];
static bool s_fail_queue = true, s_fail_timer = true;
static bool s_change_page;
static lv_timer_t s_timer;
static void (*s_tick)(lv_timer_t *);

QueueHandle_t xQueueCreate(unsigned capacity, unsigned item_size) {
    if (s_fail_queue) return NULL;
    assert(capacity == 16 && item_size <= 32);
    s_capacity = capacity; s_item_size = item_size;
    return s_items;
}
void vQueueDelete(QueueHandle_t queue) { assert(queue == s_items); ++s_deleted; }
int xQueueSend(QueueHandle_t queue, const void *item, TickType_t wait) {
    assert(queue == s_items && wait == 0); ++s_sent;
    if (s_count == s_capacity) return 0;
    memcpy(s_items[s_count++], item, s_item_size); return pdTRUE;
}
int xQueueReceive(QueueHandle_t queue, void *item, TickType_t wait) {
    assert(queue == s_items && wait == 0);
    if (!s_count) return 0;
    memcpy(item, s_items[0], s_item_size);
    --s_count;
    memmove(s_items[0], s_items[1], s_count * sizeof(s_items[0]));
    return pdTRUE;
}
lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), uint32_t period, void *user) {
    assert(period == 16 && user == NULL);
    if (s_fail_timer) return NULL;
    s_tick = callback; return &s_timer;
}
void input_test_warning(unsigned dropped) { s_warnings += dropped; }
static void deliver(bsp_btn_t button, bsp_btn_ev_t event) {
    assert(s_delivered < 32);
    s_buttons[s_delivered] = button; s_events[s_delivered++] = event;
    if (s_change_page) physical_input_advance_page();
}
int main(void) {
    physical_input_enqueue(BSP_BTN_OK, BSP_BTN_CLICK, NULL);
    assert(s_sent == 0);
    assert(!physical_input_init(NULL));
    assert(!physical_input_init(deliver)); // Queue allocation failure.
    s_fail_queue = false;
    assert(!physical_input_init(deliver) && s_deleted == 1); // Timer failure cleans up.
    s_fail_timer = false;
    assert(physical_input_init(deliver));
    assert(!physical_input_init(deliver));
    physical_input_enqueue(BSP_BTN_OK, BSP_BTN_CLICK, NULL);
    s_tick(&s_timer);
    assert(s_sent == 0 && s_delivered == 0); // Startup remains gated.
    physical_input_start();
    physical_input_enqueue(BSP_BTN_UP, BSP_BTN_PRESS, NULL);
    physical_input_enqueue(BSP_BTN_UP, BSP_BTN_CLICK, NULL);
    physical_input_enqueue(BSP_BTN_DOWN, BSP_BTN_DOUBLE, NULL);
    assert(s_delivered == 0); // Button task cannot run actions synchronously.
    s_tick(&s_timer);
    assert(s_delivered == 3 && s_buttons[0] == BSP_BTN_UP && s_events[0] == BSP_BTN_PRESS);
    assert(s_buttons[1] == BSP_BTN_UP && s_events[1] == BSP_BTN_CLICK);
    assert(s_buttons[2] == BSP_BTN_DOWN && s_events[2] == BSP_BTN_DOUBLE);
    s_delivered = 0; s_change_page = true;
    physical_input_enqueue(BSP_BTN_OK, BSP_BTN_LONG, NULL);
    physical_input_enqueue(BSP_BTN_DOWN, BSP_BTN_CLICK, NULL);
    s_tick(&s_timer);
    assert(s_delivered == 1 && s_events[0] == BSP_BTN_LONG && s_count == 0);
    s_change_page = false;
    physical_input_enqueue(BSP_BTN_DOWN, BSP_BTN_CLICK, NULL);
    s_tick(&s_timer);
    assert(s_delivered == 2 && s_buttons[1] == BSP_BTN_DOWN);
    s_delivered = 0;
    for (unsigned i = 0; i < 20; ++i) physical_input_enqueue(BSP_BTN_OK, BSP_BTN_CLICK, NULL);
    s_tick(&s_timer);
    assert(s_delivered == 16 && s_warnings == 4 && s_count == 0);
    s_tick(&s_timer);
    assert(s_warnings == 4); // Overflow report is consumed once, queue recovers.
    puts("Physical input: PASS (nonblocking queue, ordering, startup, page epochs, overflow, allocation failures)");
    return 0;
}
