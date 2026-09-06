#pragma once

#include "bsp_button.h"
#include <stdbool.h>

typedef void (*physical_input_handler_t)(bsp_btn_t button, bsp_btn_ev_t event);

// App-lifetime dispatcher. Initialize/start/advance only in the LVGL context.
// Input is ignored until start; advance invalidates events queued by an old page.
bool physical_input_init(physical_input_handler_t handler);
void physical_input_start(void);
void physical_input_advance_page(void);

// BSP callback: safe on the button timer task, always non-blocking.
void physical_input_enqueue(bsp_btn_t button, bsp_btn_ev_t event, void *user);
