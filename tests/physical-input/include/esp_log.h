#pragma once
void input_test_warning(unsigned dropped);
#define ESP_LOGW(tag, format, dropped) input_test_warning(dropped)
