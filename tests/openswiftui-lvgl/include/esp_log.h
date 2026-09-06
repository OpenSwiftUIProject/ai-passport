#pragma once
#include <stdio.h>
#define ESP_LOGI(tag, format, ...) printf("HOST %s: " format "\n", tag, __VA_ARGS__)
