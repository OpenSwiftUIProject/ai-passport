#pragma once
#include <stdint.h>
typedef struct lv_timer { unsigned unused; } lv_timer_t;
lv_timer_t *lv_timer_create(void (*callback)(lv_timer_t *), uint32_t period, void *user);
